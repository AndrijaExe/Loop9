#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop9BackendAuthSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnLoop9AuthSessionReady);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoop9AuthSessionFailed, const FString& /*Reason*/);

/**
 * Exchanges the local Steam auth session ticket for a short-lived backend
 * session token (POST /api/auth/steam). While a valid session token exists,
 * chat requests use it instead of the shared game token, and the backend
 * derives the player identity from the verified Steam ID.
 *
 * On non-Steam builds (or when Steam is unavailable) this quietly does
 * nothing and the game can fall back to the legacy X-Game-Token flow when
 * explicitly configured.
 */
UCLASS(Config = Game)
class LOOP9_API ULoop9BackendAuthSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Optional explicit override; if empty it is derived from the chat endpoint. */
	UPROPERTY(Config)
	FString AuthEndpoint;

	/**
	 * Production default: chat must wait for a verified Steam session.
	 * Set false only in explicit non-production configurations that use X-Game-Token.
	 */
	UPROPERTY(Config)
	bool bRequireSteamSession = true;

	/** Derives the auth endpoint; the ticket exchange starts on demand via EnsureSession. */
	void ConfigureFromChatEndpoint(const FString& ChatEndpoint);

	/** Starts or joins a token refresh. Returns false when configuration/backoff prevents an attempt. */
	bool EnsureSession();

	/** Current backend session token, or empty when unauthenticated. */
	FString GetSessionToken() const;

	/** Verified backend player id ("steam-<id64>"), or empty when unauthenticated. */
	FString GetAuthPlayerId() const { return AuthPlayerId; }

	bool HasValidSession() const;

	bool RequiresSteamSession() const { return bRequireSteamSession; }

	/** Drops the current session and re-authenticates (e.g. after a 403 from chat). */
	bool InvalidateAndReauth();

	FOnLoop9AuthSessionReady OnSessionReady;
	FOnLoop9AuthSessionFailed OnSessionFailed;

	virtual void Deinitialize() override;

private:
	void RequestSessionToken();
	void HandleAuthFailure(const FString& Reason);
	void ClearAuthTimeout();
	FString ResolveSteamAuthTicket() const;

	FString SessionToken;
	FString AuthPlayerId;
	int64 SessionExpiresAtUnix = 0;
	bool bRequestInFlight = false;
	double LastAttemptSeconds = 0.0;
	int32 ConsecutiveFailures = 0;
	int32 TicketRetryCount = 0;
	uint64 AuthRequestGeneration = 0;
	double AuthDeadlineSeconds = 0.0;
	FTimerHandle AuthTimeoutHandle;
	FTimerHandle TicketRetryHandle;
};
