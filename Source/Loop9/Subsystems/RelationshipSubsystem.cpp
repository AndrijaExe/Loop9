#include "Subsystems/RelationshipSubsystem.h"

#include "Loop/LoopEndingEvaluator.h"

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
	Dependency += 0.03f;
	Cooperation += 0.01f;
	Trust += 0.005f;
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

	ClampStateValues();
}

void URelationshipSubsystem::RegisterPlayerMessage(const FString& Message)
{
	const FString Lower = Message.ToLower();

	if (Lower.Contains(TEXT("hvala")) || Lower.Contains(TEXT("thank")) || Lower.Contains(TEXT("please"))
		|| Lower.Contains(TEXT("molim")) || Lower.Contains(TEXT("izvini")) || Lower.Contains(TEXT("sorry"))
		|| Lower.Contains(TEXT("bravo")) || Lower.Contains(TEXT("svaka cast")) || Lower.Contains(TEXT("dobar si"))
		|| Lower.Contains(TEXT("pomogao")) || Lower.Contains(TEXT("helped")) || Lower.Contains(TEXT("legendo")))
	{
		Kindness += 0.04f;
	}

	if (Lower.Contains(TEXT("idiot")) || Lower.Contains(TEXT("glup")) || Lower.Contains(TEXT("stupid"))
		|| Lower.Contains(TEXT("mrzim")) || Lower.Contains(TEXT("hate")) || Lower.Contains(TEXT("debil"))
		|| Lower.Contains(TEXT("retard")) || Lower.Contains(TEXT("kreten")) || Lower.Contains(TEXT("budalo"))
		|| Lower.Contains(TEXT("odjebi")) || Lower.Contains(TEXT("mars")))
	{
		Kindness -= 0.08f;
	}

	if (Lower.Contains(TEXT("vidim")) || Lower.Contains(TEXT("i see")) || Lower.Contains(TEXT("anomal"))
		|| Lower.Contains(TEXT("nema")) || Lower.Contains(TEXT("there is")) || Lower.Contains(TEXT("there isn't")))
	{
		Cooperation += 0.03f;
	}

	if (Lower.Contains(TEXT("sta da radim")) || Lower.Contains(TEXT("what should i do"))
		|| Lower.Contains(TEXT("reci mi")) || Lower.Contains(TEXT("tell me")))
	{
		Dependency += 0.05f;
	}

	if (Lower.Contains(TEXT("laz")) || Lower.Contains(TEXT("laž")) || Lower.Contains(TEXT("ne verujem"))
		|| Lower.Contains(TEXT("don't trust")) || Lower.Contains(TEXT("sumnj")) || Lower.Contains(TEXT("sus")))
	{
		Suspicion += 0.04f;
		Trust -= 0.03f;
	}

	ClampStateValues();
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
