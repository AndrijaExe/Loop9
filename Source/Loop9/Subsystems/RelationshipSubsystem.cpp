#include "Subsystems/RelationshipSubsystem.h"

#include "Loop/LoopEndingEvaluator.h"
#include "Runtime/Loop9RunEventCards.h"
#include "Runtime/Loop9RuntimePolicies.h"

namespace
{
	constexpr float DefaultTrust = 0.5f;
	constexpr float DefaultKindness = 0.5f;
	constexpr float DefaultCooperation = 0.5f;
	constexpr float DefaultSuspicion = 0.2f;
	constexpr float DefaultDependency = 0.2f;
	constexpr float DefaultAIStability = 1.0f;
}

void URelationshipSubsystem::RegisterAIInteraction()
{
	TotalAIInteractions++;
	AIInteractionsThisLoop++;
	// Talking still builds a little rapport. Dependency itself comes from [STATE].
	// Halved vs the old 1-message-per-loop rates so two calls on one floor
	// do not count as two full shifts of trust.
	Cooperation += 0.005f;
	Trust += 0.003f;
	ClampStateValues();
}

void URelationshipSubsystem::ApplyAIDiagnosedSuspicionDelta(int32 Delta)
{
	const int32 ClampedDelta = FMath::Clamp(Delta, -1, 1);

	if (ClampedDelta > 0)
	{
		Suspicion += 0.05f;
		Trust -= 0.03f;
	}
	else if (ClampedDelta < 0)
	{
		Suspicion -= 0.04f;
		Trust += 0.02f;
	}

	ClampStateValues();
}

void URelationshipSubsystem::ApplyAIDiagnosedDependencyDelta(int32 Delta)
{
	const int32 ClampedDelta = FMath::Clamp(Delta, -1, 1);

	if (ClampedDelta > 0)
	{
		Dependency += 0.07f;
	}
	else if (ClampedDelta < 0)
	{
		Dependency -= 0.05f;
	}

	ClampStateValues();
}

void URelationshipSubsystem::ApplyAIDiagnosedKindnessDelta(int32 Delta)
{
	const int32 ClampedDelta = FMath::Clamp(Delta, -1, 1);

	if (ClampedDelta > 0)
	{
		Kindness += 0.05f;
	}
	else if (ClampedDelta < 0)
	{
		Kindness -= 0.08f;
	}

	ClampStateValues();
}

void URelationshipSubsystem::ResetRelationshipState()
{
	TotalResets = 0;
	TotalAdvances = 0;
	TotalAIInteractions = 0;

	Trust = DefaultTrust;
	Kindness = DefaultKindness;
	Cooperation = DefaultCooperation;
	Suspicion = DefaultSuspicion;
	Dependency = DefaultDependency;
	AI_Stability = DefaultAIStability;

	RunEvents.Reset();
	AIInteractionsThisLoop = 0;

	ClampStateValues();
}

void URelationshipSubsystem::RecordCall(int32 LoopIndex, int32 KindnessDelta, int32 SuspicionDelta, int32 DependencyDelta)
{
	FRunEvent Event;
	Event.Type = ERunEventType::Call;
	Event.LoopIndex = LoopIndex;
	Event.KindnessDelta = KindnessDelta;
	Event.SuspicionDelta = SuspicionDelta;
	Event.DependencyDelta = DependencyDelta;
	AppendEvent(MoveTemp(Event));
}

void URelationshipSubsystem::RecordLift(int32 LoopIndex, bool bWasCorrect, bool bAnomalyExisted)
{
	FRunEvent Event;
	Event.Type = bWasCorrect ? ERunEventType::CorrectLift : ERunEventType::WrongLift;
	Event.LoopIndex = LoopIndex;
	Event.bAnomalyExisted = bAnomalyExisted;
	AppendEvent(MoveTemp(Event));
}

void URelationshipSubsystem::RecordEnding(int32 LoopIndex, ELoopEndingType EndingType)
{
	FRunEvent Event;
	Event.Type = ERunEventType::Ending;
	Event.LoopIndex = LoopIndex;
	Event.EndingType = EndingType;
	AppendEvent(MoveTemp(Event));
}

TArray<FRunEventCard> URelationshipSubsystem::BuildRunEventCards() const
{
	return Loop9RunEventCards::BuildAll(RunEvents);
}

void URelationshipSubsystem::AppendEvent(FRunEvent Event)
{
	Loop9RuntimePolicies::AppendRunEvent(RunEvents, MoveTemp(Event));
}

void URelationshipSubsystem::NotifyLoopLeft()
{
	if (AIInteractionsThisLoop <= 0)
	{
		Dependency -= 0.04f;
		ClampStateValues();
	}

	AIInteractionsThisLoop = 0;
}

void URelationshipSubsystem::RegisterLoopDecision(bool bWasCorrect, bool bAnomalyExisted, EButtonType ButtonType)
{
	if (bWasCorrect)
	{
		Trust += 0.04f;
		Suspicion -= 0.02f;
	}
	else
	{
		Trust -= 0.08f;
		Suspicion += 0.08f;
		AI_Stability -= 0.03f;
	}

	if ((bAnomalyExisted && ButtonType == EButtonType::Reset)
		|| (!bAnomalyExisted && ButtonType == EButtonType::Increment))
	{
		Cooperation += 0.02f;
	}

	ClampStateValues();
}

void URelationshipSubsystem::TickAIStabilityDecay()
{
	AI_Stability -= 0.01f;
	AI_Stability += (Kindness > 0.7f ? 0.005f : 0.0f);
	AI_Stability += (Cooperation > 0.7f ? 0.005f : 0.0f);

	ClampStateValues();
}

void URelationshipSubsystem::ClampStateValues()
{
	Trust = FMath::Clamp(Trust, 0.0f, 1.0f);
	Kindness = FMath::Clamp(Kindness, 0.0f, 1.0f);
	Cooperation = FMath::Clamp(Cooperation, 0.0f, 1.0f);
	Suspicion = FMath::Clamp(Suspicion, 0.0f, 1.0f);
	Dependency = FMath::Clamp(Dependency, 0.0f, 1.0f);
	AI_Stability = FMath::Clamp(AI_Stability, 0.0f, 1.0f);

	UE_LOG(LogTemp, Log, TEXT("State -> Trust: %.2f | Kindness: %.2f | Cooperation: %.2f | Suspicion: %.2f | Dependency: %.2f | AI_Stability: %.2f"),
		Trust, Kindness, Cooperation, Suspicion, Dependency, AI_Stability);
}

FEndingEvaluationContext URelationshipSubsystem::BuildEndingContext() const
{
	return {
		Trust,
		Kindness,
		Cooperation,
		Suspicion,
		Dependency,
		AI_Stability,
		TotalAdvances,
		TotalAIInteractions
	};
}
