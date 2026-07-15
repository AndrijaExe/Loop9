#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop/LoopTypes.h"
#include "Loop9TelemetrySubsystem.generated.h"

/**
 * Fire-and-forget end-of-run telemetry (POST /api/telemetry/run).
 * Anonymous balancing data only: which ending, how many resets, how many AI
 * messages. Failures are silent - telemetry must never affect gameplay.
 *
 * Configured from AI_Friend::BeginPlay with the chat endpoint and legacy
 * token; the session token (Steam auth) is picked up automatically when
 * available.
 */
UCLASS()
class LOOP9_API ULoop9TelemetrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Derives the telemetry endpoint from the chat endpoint and stores the fallback token. */
	void ConfigureFromChatEndpoint(const FString& ChatEndpoint, const FString& InGameToken);

	/** Sends the end-of-run ping. Safe to call when unconfigured (no-op). */
	void SendRunFinished(ELoopEndingType EndingType, int32 TotalResets, int32 TotalAIInteractions);

	/** Steamworks-independent ending id used by the backend ("escape_together"...). */
	static FString EndingTelemetryId(ELoopEndingType EndingType);

private:
	FString TelemetryEndpoint;
	FString GameToken;
};
