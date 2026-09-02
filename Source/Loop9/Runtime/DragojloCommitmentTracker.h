#pragma once

#include "CoreMinimal.h"
#include "Loop/LoopTypes.h"

/**
 * Pure per-run reducer for Dragojlo's structured commitments.
 * Time is supplied by the caller so behavior is deterministic in tests.
 */
class LOOP9_API FDragojloCommitmentTracker
{
public:
	void ApplyAdvice(
		EDragojloAdviceMode Mode,
		EDragojloLiftAdvice LiftAdvice,
		const FString& SuggestedZone,
		const FString& CommitmentId,
		int32 SuspicionDelta,
		int32 DependencyDelta,
		double NowSeconds);

	void ApplyDecision(EButtonType ButtonType);
	bool OnZoneEntered(FName ZoneId, double NowSeconds);
	void Reset();

	const FDragojloCommitmentState& GetState() const { return State; }
	EDragojloLiftAdvice GetPendingLiftAdvice() const { return PendingLiftAdvice; }
	EDragojloAdviceMode GetPendingLiftAdviceMode() const { return PendingLiftAdviceMode; }

private:
	FDragojloCommitmentState State;
	EDragojloLiftAdvice PendingLiftAdvice = EDragojloLiftAdvice::None;
	EDragojloAdviceMode PendingLiftAdviceMode = EDragojloAdviceMode::None;
	FName ActiveDecoyZoneId = NAME_None;
	double DecoyTrackingStartedAt = 0.0;
};
