#include "Loop/LoopEndingEvaluator.h"

namespace
{
	// A first-time player who experiments with chat can establish a relationship
	// without farming a locale phrase. Dependency comes from AI [STATE].
	constexpr int32 MinAIInteractionsForRelationshipEndings = 3;

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

	// Rare: a long, warm but unhealthy relationship during a difficult run.
	if (Context.Trust >= 0.58f
		&& Context.Kindness >= 0.60f
		&& Context.Cooperation >= 0.65f
		&& Context.Dependency >= 0.62f
		&& Context.AIStability <= 0.70f
		&& Context.TotalAIInteractions >= 11)
	{
		return ELoopEndingType::TheReplacement;
	}

	// The player remained humane and cooperative while repeated mistakes made
	// the AI unstable. Six conversations are enough to discover this naturally.
	if (Context.Kindness >= 0.58f
		&& Context.Cooperation >= 0.62f
		&& Context.Trust >= 0.48f
		&& Context.AIStability <= 0.72f
		&& Context.TotalAdvances >= 9
		&& Context.TotalAIInteractions >= 6)
	{
		return ELoopEndingType::MergedMemory;
	}

	// Relied on Dragojlo while treating him poorly. This is reachable with
	// roughly five or six chats and two clearly hostile responses.
	if (Context.Trust >= 0.58f
		&& Context.Kindness <= 0.40f
		&& Context.Cooperation >= 0.58f
		&& Context.Dependency >= 0.35f
		&& Context.AIStability >= 0.55f
		&& Context.TotalAIInteractions >= 5)
	{
		return ELoopEndingType::ColdBetrayal;
	}

	// Frequently asked Dragojlo to decide, in any language. Chats no longer add
	// dependency themselves: it is 0.2 + 0.07 per DEPENDENCY=1, -0.05 per
	// DEPENDENCY=-1, -0.04 per floor left without calling. Five positive
	// diagnoses clear 0.53 only if the player calls on nearly every floor —
	// cramming the eight chats into a few floors stays below the gate.
	if (Context.Trust >= 0.55f
		&& Context.Kindness >= 0.42f
		&& Context.Dependency >= 0.53f
		&& Context.Suspicion <= 0.38f
		&& Context.AIStability >= 0.55f
		&& Context.TotalAIInteractions >= 8)
	{
		return ELoopEndingType::ObedientFool;
	}

	// Expected positive first-run ending: several conversations, mostly correct
	// decisions, and basic cooperation — no hidden phrase memorization required.
	if (Context.Trust >= 0.62f
		&& Context.Kindness >= 0.48f
		&& Context.Cooperation >= 0.60f
		&& Context.Dependency < 0.53f
		&& Context.AIStability >= 0.70f
		&& Context.TotalAIInteractions >= 4)
	{
		return ELoopEndingType::EscapeTogether;
	}

	if (Context.Suspicion >= 0.52f || Context.Trust <= 0.42f)
	{
		return ELoopEndingType::ParanoidSurvivor;
	}

	return ELoopEndingType::ParanoidSurvivor;
}
