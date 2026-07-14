#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop9BackendAuthSubsystem.generated.h"

/**
 * Exchanges the local Steam auth session ticket for a short-lived backend
 * session token (POST /api/auth/steam). While a valid session token exists,
 * chat requests use it instead of the shared game token, and the backend
 * derives the player identity from the verified Steam ID.
 *
 * On non-Steam builds (or when Steam is unavailable) this quietly does
 * nothing and the game falls back to the legacy X-Game-Token flow.
 */
UCLASS(Config = Game)
class LOOP9_API ULoop9BackendAuthSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Optional explicit override; if empty it is derived from the chat endpoint. */
	UPROPERTY(Config)
	FString AuthEndpoint;

	/** Derives the auth endpoint from the chat endpoint and starts the ticket exchange if needed. */
	void ConfigureFromChatEndpoint(const FString& ChatEndpoint);

	/** Kicks off a token refresh when there is no valid session. Safe to call often. */
	void EnsureSession();

	/** Current backend session token, or empty when unauthenticated. */
	FString GetSessionToken() const;

	/** Verified backend player id ("steam-<id64>"), or empty when unauthenticated. */
	FString GetAuthPlayerId() const { return AuthPlayerId; }

	bool HasValidSession() const;

	/** Drops the current session and re-authenticates (e.g. after a 403 from chat). */
	void InvalidateAndReauth();

private:
	void RequestSessionToken();
	FString ResolveSteamAuthTicket() const;

	FString SessionToken;
	FString AuthPlayerId;
	int64 SessionExpiresAtUnix = 0;
	bool bRequestInFlight = false;
	double LastAttemptSeconds = 0.0;
};
