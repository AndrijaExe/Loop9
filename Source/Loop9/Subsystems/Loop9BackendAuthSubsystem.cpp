#include "Subsystems/Loop9BackendAuthSubsystem.h"

#include "AI/Services/Loop9BackendEndpointUtils.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "Misc/Guid.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "TimerManager.h"

namespace
{
	// Refresh slightly before actual expiry so an in-flight chat never races the deadline.
	constexpr int64 ExpiryMarginSeconds = 120;
	// Back off between failed attempts so a dead backend is not hammered every chat.
	constexpr double RetryBackoffSeconds = 30.0;
	constexpr int32 MaxConsecutiveFailures = 3;
	constexpr int32 MaxTicketRetryCount = 40;
	constexpr float TicketRetryDelaySeconds = 1.0f;
	const TCHAR* SteamWebApiTokenType = TEXT("WebAPI:Loop9");
}

void ULoop9BackendAuthSubsystem::Deinitialize()
{
	++AuthRequestGeneration;
	bRequestInFlight = false;
	bSteamTicketRequestInFlight = false;
	ClearAuthTimeout();
	OnSessionReady.Clear();
	OnSessionFailed.Clear();
	Super::Deinitialize();
}

void ULoop9BackendAuthSubsystem::ConfigureFromChatEndpoint(const FString& ChatEndpoint)
{
	if (AuthEndpoint.IsEmpty() && !ChatEndpoint.IsEmpty())
	{
		AuthEndpoint = Loop9BackendEndpointUtils::DeriveSteamAuthEndpoint(ChatEndpoint);
	}

	const FString EnvAuthEndpoint = FPlatformMisc::GetEnvironmentVariable(TEXT("LOOP9_AUTH_ENDPOINT"));
	if (!EnvAuthEndpoint.IsEmpty())
	{
		AuthEndpoint = EnvAuthEndpoint.TrimStartAndEnd();
	}

	// Authentication is demand-driven by chat. Starting here can race Steam's
	// asynchronous local login and create a backoff before the first message.
}

bool ULoop9BackendAuthSubsystem::HasValidSession() const
{
	return !SessionToken.IsEmpty()
		&& FDateTime::UtcNow().ToUnixTimestamp() < (SessionExpiresAtUnix - ExpiryMarginSeconds);
}

FString ULoop9BackendAuthSubsystem::GetSessionToken() const
{
	return HasValidSession() ? SessionToken : FString();
}

bool ULoop9BackendAuthSubsystem::EnsureSession()
{
	if (HasValidSession() || bRequestInFlight)
	{
		return true;
	}
	if (AuthEndpoint.IsEmpty())
	{
		return false;
	}

	if (ConsecutiveFailures >= MaxConsecutiveFailures)
	{
		const double Now = FPlatformTime::Seconds();
		if (LastAttemptSeconds > 0.0 && (Now - LastAttemptSeconds) < RetryBackoffSeconds)
		{
			return false;
		}
		// Allow another bounded burst after the cooldown window.
		ConsecutiveFailures = 0;
	}

	const double Now = FPlatformTime::Seconds();
	if (LastAttemptSeconds > 0.0 && (Now - LastAttemptSeconds) < RetryBackoffSeconds && ConsecutiveFailures > 0)
	{
		return false;
	}
	LastAttemptSeconds = Now;

	ClearAuthTimeout();
	bRequestInFlight = true;
	TicketRetryCount = 0;
	const uint64 RequestGeneration = ++AuthRequestGeneration;
	AuthDeadlineSeconds = Now + AuthTimeoutSeconds;

	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<ULoop9BackendAuthSubsystem> WeakThis(this);
		World->GetTimerManager().SetTimer(
			AuthTimeoutHandle,
			[WeakThis, RequestGeneration]()
			{
				if (!WeakThis.IsValid())
				{
					return;
				}

				ULoop9BackendAuthSubsystem* Self = WeakThis.Get();
				if (Self->bRequestInFlight && Self->AuthRequestGeneration == RequestGeneration)
				{
					Self->HandleAuthFailure(FString::Printf(
						TEXT("auth timed out after %.0fs"),
						AuthTimeoutSeconds));
				}
			},
			static_cast<float>(AuthTimeoutSeconds),
			false);
	}

	RequestSessionToken();
	return bRequestInFlight || HasValidSession();
}

bool ULoop9BackendAuthSubsystem::InvalidateAndReauth()
{
	// Invalidate any late callback from the previous exchange.
	++AuthRequestGeneration;
	bRequestInFlight = false;
	bSteamTicketRequestInFlight = false;
	ClearAuthTimeout();
	SessionToken.Reset();
	AuthPlayerId.Reset();
	SessionExpiresAtUnix = 0;
	AuthDeadlineSeconds = 0.0;
	LastAttemptSeconds = 0.0;
	ConsecutiveFailures = 0;
	TicketRetryCount = 0;
	return EnsureSession();
}

void ULoop9BackendAuthSubsystem::ClearAuthTimeout()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AuthTimeoutHandle);
		World->GetTimerManager().ClearTimer(TicketRetryHandle);
	}
	else
	{
		AuthTimeoutHandle.Invalidate();
		TicketRetryHandle.Invalidate();
	}
}

void ULoop9BackendAuthSubsystem::HandleAuthFailure(const FString& Reason)
{
	ClearAuthTimeout();
	bRequestInFlight = false;
	bSteamTicketRequestInFlight = false;
	TicketRetryCount = 0;
	AuthDeadlineSeconds = 0.0;
	++ConsecutiveFailures;
	UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: %s"), *Reason);
	OnSessionFailed.Broadcast(Reason);
}

void ULoop9BackendAuthSubsystem::ScheduleSteamTicketRetry(const FString& Reason)
{
	const double RemainingSeconds = AuthDeadlineSeconds - FPlatformTime::Seconds();
	if (TicketRetryCount < MaxTicketRetryCount
		&& RemainingSeconds > TicketRetryDelaySeconds + 0.5
		&& GetWorld())
	{
		++TicketRetryCount;
		UE_LOG(LogTemp, Verbose, TEXT("Loop9 auth: retrying Steam Web API ticket (%d/%d): %s"),
			TicketRetryCount, MaxTicketRetryCount, *Reason);

		TWeakObjectPtr<ULoop9BackendAuthSubsystem> WeakThis(this);
		GetWorld()->GetTimerManager().SetTimer(
			TicketRetryHandle,
			[WeakThis]()
			{
				if (WeakThis.IsValid() && WeakThis->bRequestInFlight)
				{
					WeakThis->RequestSessionToken();
				}
			},
			TicketRetryDelaySeconds,
			false);
		return;
	}

	HandleAuthFailure(FString::Printf(TEXT("no Steam Web API ticket available (%s)"), *Reason));
}

void ULoop9BackendAuthSubsystem::RequestSessionToken()
{
	if (!bRequestInFlight || bSteamTicketRequestInFlight)
	{
		return;
	}

	if ((AuthDeadlineSeconds - FPlatformTime::Seconds()) <= 0.5)
	{
		HandleAuthFailure(TEXT("auth deadline exhausted before Steam ticket"));
		return;
	}

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(FName(TEXT("Steam")));
	if (!OnlineSubsystem)
	{
		ScheduleSteamTicketRetry(TEXT("OnlineSubsystemSteam unavailable"));
		return;
	}

	IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		ScheduleSteamTicketRetry(TEXT("Steam identity interface missing"));
		return;
	}

	const ELoginStatus::Type LoginStatus = Identity->GetLoginStatus(0);
	if (LoginStatus != ELoginStatus::LoggedIn && TicketRetryCount == 0)
	{
		// Steam handles login via the running client; AutoLogin just refreshes local state.
		Identity->AutoLogin(0);
	}

	if (Identity->GetLoginStatus(0) != ELoginStatus::LoggedIn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: Steam user not logged in (status=%d). Open Steam client and use Play > Standalone Game."),
			static_cast<int32>(Identity->GetLoginStatus(0)));
		ScheduleSteamTicketRetry(TEXT("Steam user not logged in yet"));
		return;
	}

	bSteamTicketRequestInFlight = true;
	const uint64 RequestGeneration = AuthRequestGeneration;
	TWeakObjectPtr<ULoop9BackendAuthSubsystem> WeakThis(this);
	Identity->GetLinkedAccountAuthToken(
		0,
		SteamWebApiTokenType,
		IOnlineIdentity::FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda(
			[WeakThis, RequestGeneration](int32, bool bWasSuccessful, const FExternalAuthToken& AuthToken)
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleSteamWebApiTicket(
						RequestGeneration,
						bWasSuccessful,
						AuthToken.TokenString);
				}
			}));
}

void ULoop9BackendAuthSubsystem::HandleSteamWebApiTicket(
	uint64 RequestGeneration,
	bool bWasSuccessful,
	const FString& Ticket)
{
	if (!bRequestInFlight || AuthRequestGeneration != RequestGeneration)
	{
		return;
	}

	bSteamTicketRequestInFlight = false;
	if (!bWasSuccessful || Ticket.IsEmpty())
	{
		ScheduleSteamTicketRetry(TEXT("GetLinkedAccountAuthToken returned no ticket"));
		return;
	}

	TicketRetryCount = 0;
	UE_LOG(LogTemp, Log, TEXT("Loop9 auth: fresh Steam Web API ticket acquired. TicketHexLen=%d"), Ticket.Len());
	ExchangeSteamTicketForSession(Ticket, RequestGeneration);
}

void ULoop9BackendAuthSubsystem::ExchangeSteamTicketForSession(
	const FString& Ticket,
	uint64 RequestGeneration)
{
	if (!bRequestInFlight || AuthRequestGeneration != RequestGeneration)
	{
		return;
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField(TEXT("ticket"), Ticket);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	const double RemainingSeconds = AuthDeadlineSeconds - FPlatformTime::Seconds();
	if (RemainingSeconds <= 0.5)
	{
		HandleAuthFailure(TEXT("auth deadline exhausted before HTTP exchange"));
		return;
	}

	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(AuthEndpoint);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	const FString RequestId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	HttpRequest->SetHeader(TEXT("X-Request-Id"), RequestId);
	HttpRequest->SetTimeout(static_cast<float>(RemainingSeconds));
	HttpRequest->SetContentAsString(Body);

	const double AuthStartedAt = AuthDeadlineSeconds - AuthTimeoutSeconds;

	TWeakObjectPtr<ULoop9BackendAuthSubsystem> WeakThis(this);
	HttpRequest->OnProcessRequestComplete().BindLambda(
		[WeakThis, RequestGeneration, RequestId, AuthStartedAt](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			ULoop9BackendAuthSubsystem* Self = WeakThis.Get();
			if (!Self->bRequestInFlight || Self->AuthRequestGeneration != RequestGeneration)
			{
				// Timed out or superseded already; ignore late responses.
				return;
			}

			Self->ClearAuthTimeout();
			Self->bRequestInFlight = false;

			if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
			{
				const int32 Code = Response.IsValid() ? Response->GetResponseCode() : 0;
				UE_LOG(LogTemp, Warning, TEXT("Loop9 auth request failed. RequestId=%s | HttpCode=%d | DurationMs=%.1f"),
					*RequestId,
					Code,
					(FPlatformTime::Seconds() - AuthStartedAt) * 1000.0);
				Self->HandleAuthFailure(FString::Printf(TEXT("session token exchange failed (HTTP %d)"), Code));
				return;
			}

			TSharedPtr<FJsonObject> JsonResponse;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid())
			{
				Self->HandleAuthFailure(TEXT("invalid auth response payload"));
				return;
			}

			FString Token;
			FString PlayerId;
			double ExpiresAt = 0.0;
			if (!JsonResponse->TryGetStringField(TEXT("token"), Token) || Token.IsEmpty()
				|| !JsonResponse->TryGetNumberField(TEXT("expires_at"), ExpiresAt))
			{
				Self->HandleAuthFailure(TEXT("auth response missing token/expires_at"));
				return;
			}
			JsonResponse->TryGetStringField(TEXT("player_id"), PlayerId);

			Self->SessionToken = Token;
			Self->AuthPlayerId = PlayerId;
			Self->SessionExpiresAtUnix = static_cast<int64>(ExpiresAt);
			Self->ConsecutiveFailures = 0;
			Self->AuthDeadlineSeconds = 0.0;

			UE_LOG(LogTemp, Log, TEXT("Loop9 auth: session token acquired. RequestId=%s | ExpiresInSeconds=%lld | DurationMs=%.1f"),
				*RequestId,
				Self->SessionExpiresAtUnix - FDateTime::UtcNow().ToUnixTimestamp(),
				(FPlatformTime::Seconds() - AuthStartedAt) * 1000.0);

			Self->OnSessionReady.Broadcast();
		});

	if (!HttpRequest->ProcessRequest())
	{
		HandleAuthFailure(TEXT("failed to start auth HTTP request"));
	}
}
