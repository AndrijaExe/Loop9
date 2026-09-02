#include "Runtime/DragojloCommitmentTracker.h"

#include "Runtime/Loop9ObservationIds.h"

void FDragojloCommitmentTracker::ApplyAdvice(
	EDragojloAdviceMode Mode,
	EDragojloLiftAdvice LiftAdvice,
	const FString& SuggestedZone,
	const FString& CommitmentId,
	int32 SuspicionDelta,
	int32 DependencyDelta,
	double NowSeconds)
{
	State.LastAdviceMode = Mode;
	State.LastSuggestedZone = SuggestedZone;

	if (!CommitmentId.IsEmpty())
	{
		State.LastCommitmentId = CommitmentId;
	}

	if (LiftAdvice != EDragojloLiftAdvice::None)
	{
		State.LastLiftAdvice = LiftAdvice;
		State.bFollowedLastLiftAdvice = false;
		PendingLiftAdvice = LiftAdvice;
		PendingLiftAdviceMode = Mode;
		++State.LiftAdviceCount;
	}

	if (Mode == EDragojloAdviceMode::MisdirectLocation)
	{
		State.bLocationMisdirectionUsed = true;
		State.bVisitedSuggestedDecoy = false;
		State.DecoyVisitSeconds = -1.0f;
		ActiveDecoyZoneId = Loop9ObservationIds::Canonicalize(SuggestedZone);
		DecoyTrackingStartedAt = FMath::Max(0.0, NowSeconds);
	}

	if (Mode == EDragojloAdviceMode::Confrontation)
	{
		State.bConfrontationResponseUsed = true;
	}

	if (Mode == EDragojloAdviceMode::WrongLift)
	{
		State.bWrongLiftUsed = true;
		++State.WrongLiftAdviceCount;
	}

	if (State.bLocationMisdirectionUsed && SuspicionDelta > 0)
	{
		State.bContradictionExposed = true;
	}

	if (Mode == EDragojloAdviceMode::Withhold && DependencyDelta > 0)
	{
		State.bPendingDecisionSurrender = true;
	}
}

void FDragojloCommitmentTracker::ApplyDecision(EButtonType ButtonType)
{
	State.bPendingDecisionSurrender = false;

	if (PendingLiftAdvice == EDragojloLiftAdvice::None)
	{
		State.LastLiftAdvice = EDragojloLiftAdvice::None;
		return;
	}

	const bool bChoseLit = ButtonType == EButtonType::Reset;
	const bool bAdviceWasLit = PendingLiftAdvice == EDragojloLiftAdvice::Lit;
	State.bFollowedLastLiftAdvice = bChoseLit == bAdviceWasLit;
	if (State.bFollowedLastLiftAdvice)
	{
		++State.FollowedLiftAdviceCount;
		if (PendingLiftAdviceMode == EDragojloAdviceMode::WrongLift)
		{
			++State.FollowedWrongLiftAdviceCount;
		}
	}

	State.LastLiftAdvice = EDragojloLiftAdvice::None;
	PendingLiftAdvice = EDragojloLiftAdvice::None;
	PendingLiftAdviceMode = EDragojloAdviceMode::None;
}

bool FDragojloCommitmentTracker::OnZoneEntered(FName ZoneId, double NowSeconds)
{
	if (State.bVisitedSuggestedDecoy
		|| ActiveDecoyZoneId.IsNone()
		|| ActiveDecoyZoneId != Loop9ObservationIds::Canonicalize(ZoneId))
	{
		return false;
	}

	State.bVisitedSuggestedDecoy = true;
	State.DecoyVisitSeconds = static_cast<float>(
		FMath::Max(0.0, NowSeconds - DecoyTrackingStartedAt));
	ActiveDecoyZoneId = NAME_None;
	return true;
}

void FDragojloCommitmentTracker::Reset()
{
	State = FDragojloCommitmentState();
	PendingLiftAdvice = EDragojloLiftAdvice::None;
	PendingLiftAdviceMode = EDragojloAdviceMode::None;
	ActiveDecoyZoneId = NAME_None;
	DecoyTrackingStartedAt = 0.0;
}
