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
	constexpr int32 MaxTicketRetryCount = 5;
	constexpr float TicketRetryDelaySeconds = 1.0f;
}

void ULoop9BackendAuthSubsystem::Deinitialize()
{
	++AuthRequestGeneration;
	bRequestInFlight = false;
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
	TicketRetryCount = 0;
	AuthDeadlineSeconds = 0.0;
	++ConsecutiveFailures;
	UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: %s"), *Reason);
	OnSessionFailed.Broadcast(Reason);
}

FString ULoop9BackendAuthSubsystem::ResolveSteamAuthTicket() const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(FName(TEXT("Steam")));
	if (!OnlineSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: OnlineSubsystemSteam not available (Steam client running? use Standalone, not PIE)."));
		return FString();
	}

	IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: Steam identity interface missing."));
		return FString();
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
		return FString();
	}

	// For the Steam subsystem this returns the hex-encoded auth session ticket.
	// NOTE: If the backend consistently rejects tickets with STEAM_TICKET_INVALID,
	// Valve may require the newer GetAuthTicketForWebApi flow for this SDK version;
	// in that case switch this call to the identity interface's web-api ticket API.
	const FString Ticket = Identity->GetAuthToken(0);
	if (Ticket.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: Steam logged in but GetAuthToken returned empty."));
	}
	return Ticket;
}

void ULoop9BackendAuthSubsystem::RequestSessionToken()
{
	const FString Ticket = ResolveSteamAuthTicket();
	if (Ticket.IsEmpty())
	{
		if (TicketRetryCount < MaxTicketRetryCount && GetWorld())
		{
			++TicketRetryCount;

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

		HandleAuthFailure(TEXT("no Steam ticket available"));
		return;
	}
	TicketRetryCount = 0;

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

	const uint64 RequestGeneration = AuthRequestGeneration;
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
