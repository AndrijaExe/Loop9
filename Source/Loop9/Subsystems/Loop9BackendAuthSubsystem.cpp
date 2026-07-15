#include "Subsystems/Loop9BackendAuthSubsystem.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"

namespace
{
	// Refresh slightly before actual expiry so an in-flight chat never races the deadline.
	constexpr int64 ExpiryMarginSeconds = 120;
	// Back off between failed attempts so a dead backend is not hammered every chat.
	constexpr double RetryBackoffSeconds = 30.0;
}

void ULoop9BackendAuthSubsystem::ConfigureFromChatEndpoint(const FString& ChatEndpoint)
{
	if (AuthEndpoint.IsEmpty() && !ChatEndpoint.IsEmpty())
	{
		AuthEndpoint = ChatEndpoint.Replace(TEXT("/api/chat"), TEXT("/api/auth/steam"));
	}

	const FString EnvAuthEndpoint = FPlatformMisc::GetEnvironmentVariable(TEXT("LOOP9_AUTH_ENDPOINT"));
	if (!EnvAuthEndpoint.IsEmpty())
	{
		AuthEndpoint = EnvAuthEndpoint;
	}

	EnsureSession();
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

void ULoop9BackendAuthSubsystem::EnsureSession()
{
	if (HasValidSession() || bRequestInFlight || AuthEndpoint.IsEmpty())
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	if (LastAttemptSeconds > 0.0 && (Now - LastAttemptSeconds) < RetryBackoffSeconds)
	{
		return;
	}
	LastAttemptSeconds = Now;

	RequestSessionToken();
}

void ULoop9BackendAuthSubsystem::InvalidateAndReauth()
{
	SessionToken.Reset();
	AuthPlayerId.Reset();
	SessionExpiresAtUnix = 0;
	LastAttemptSeconds = 0.0;
	EnsureSession();
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
	if (LoginStatus != ELoginStatus::LoggedIn)
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
		UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: no Steam ticket available, staying on legacy game token."));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField(TEXT("ticket"), Ticket);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(AuthEndpoint);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(Body);

	bRequestInFlight = true;

	TWeakObjectPtr<ULoop9BackendAuthSubsystem> WeakThis(this);
	HttpRequest->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			ULoop9BackendAuthSubsystem* Self = WeakThis.Get();
			Self->bRequestInFlight = false;

			if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
			{
				const int32 Code = Response.IsValid() ? Response->GetResponseCode() : 0;
				UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: session token exchange failed (HTTP %d)."), Code);
				return;
			}

			TSharedPtr<FJsonObject> JsonResponse;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: invalid auth response payload."));
				return;
			}

			FString Token;
			FString PlayerId;
			double ExpiresAt = 0.0;
			if (!JsonResponse->TryGetStringField(TEXT("token"), Token) || Token.IsEmpty()
				|| !JsonResponse->TryGetNumberField(TEXT("expires_at"), ExpiresAt))
			{
				UE_LOG(LogTemp, Warning, TEXT("Loop9 auth: auth response missing token/expires_at."));
				return;
			}
			JsonResponse->TryGetStringField(TEXT("player_id"), PlayerId);

			Self->SessionToken = Token;
			Self->AuthPlayerId = PlayerId;
			Self->SessionExpiresAtUnix = static_cast<int64>(ExpiresAt);

			UE_LOG(LogTemp, Log, TEXT("Loop9 auth: session token acquired (expires in %llds)."),
				Self->SessionExpiresAtUnix - FDateTime::UtcNow().ToUnixTimestamp());
		});

	HttpRequest->ProcessRequest();
}
