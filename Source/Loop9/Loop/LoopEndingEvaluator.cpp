#include "Loop/LoopEndingEvaluator.h"

namespace
{
	// Endings that require a real relationship with the AI need at least this many chat turns.
	constexpr int32 MinAIInteractionsForRelationshipEndings = 5;

	bool HasMeaningfulAIRelationship(const FEndingEvaluationContext& Context)
	{
		return Context.TotalAIInteractions >= MinAIInteractionsForRelationshipEndings;
	}
}

ELoopEndingType FLoopEndingEvaluator::Evaluate(const FEndingEvaluationContext& Context)
{
	// Never (or barely) talked to the AI — you escaped alone, not as anyone's puppet.
	if (!HasMeaningfulAIRelationship(Context))
	{
		return ELoopEndingType::ParanoidSurvivor;
	}

	if (Context.Trust >= 0.75f
		&& Context.Kindness >= 0.70f
		&& Context.Cooperation >= 0.70f
		&& Context.Dependency >= 0.80f
		&& Context.AIStability <= 0.45f
		&& Context.TotalAIInteractions >= 10)
	{
		return ELoopEndingType::TheReplacement;
	}

	if (Context.Kindness >= 0.70f
		&& Context.Cooperation >= 0.70f
		&& Context.Trust >= 0.55f
		&& Context.AIStability <= 0.35f
		&& Context.TotalAdvances >= 6
		&& Context.TotalAIInteractions >= 8)
	{
		return ELoopEndingType::MergedMemory;
	}

	if (Context.Trust >= 0.60f
		&& Context.Kindness >= 0.65f
		&& Context.Cooperation >= 0.65f
		&& Context.Dependency <= 0.65f
		&& Context.AIStability >= 0.45f
		&& Context.TotalAIInteractions >= 6)
	{
		return ELoopEndingType::EscapeTogether;
	}

	if (Context.Trust >= 0.45f
		&& Context.Dependency >= 0.80f
		&& Context.Suspicion <= 0.35f
		&& Context.AIStability <= 0.80f
		&& Context.TotalAIInteractions >= MinAIInteractionsForRelationshipEndings)
	{
		return ELoopEndingType::ObedientFool;
	}

	if (Context.Trust >= 0.65f
		&& Context.Kindness <= 0.35f
		&& Context.Cooperation >= 0.50f
		&& Context.Dependency >= 0.55f
		&& Context.AIStability >= 0.50f
		&& Context.TotalAIInteractions >= MinAIInteractionsForRelationshipEndings)
	{
		return ELoopEndingType::ColdBetrayal;
	}

	if (Context.Suspicion >= 0.70f && Context.Trust <= 0.40f)
	{
		return ELoopEndingType::ParanoidSurvivor;
	}

	return ELoopEndingType::ParanoidSurvivor;
}
