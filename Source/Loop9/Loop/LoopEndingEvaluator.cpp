#include "Loop/LoopEndingEvaluator.h"

namespace
{
	// A first-time player who experiments with chat can establish a relationship
	// without knowing hidden keywords or repeatedly farming dialogue.
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

	// Frequently asked for instructions and accepted them without suspicion.
	// Eight ordinary chats plus one or two "tell me what to do" messages suffice.
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
