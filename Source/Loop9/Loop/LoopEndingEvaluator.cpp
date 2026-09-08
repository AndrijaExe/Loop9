#include "Loop/LoopEndingEvaluator.h"

namespace
{
	constexpr int32 MinAIInteractionsForRelationshipEndings = 3;

	float Ramp01(float Value, float ZeroAt, float OneAt)
	{
		if (FMath::IsNearlyEqual(ZeroAt, OneAt))
		{
			return Value >= OneAt ? 1.0f : 0.0f;
		}
		return FMath::Clamp((Value - ZeroAt) / (OneAt - ZeroAt), 0.0f, 1.0f);
	}

	// 0 below Threshold-Slack, 1 at/above Threshold.
	float High(float Value, float Threshold, float Slack)
	{
		return Ramp01(Value, Threshold - Slack, Threshold);
	}

	// 1 at/below Threshold, 0 at/above Threshold+Slack.
	float Low(float Value, float Threshold, float Slack)
	{
		return Ramp01(Value, Threshold + Slack, Threshold);
	}

	struct FEndingScore
	{
		ELoopEndingType Type = ELoopEndingType::ParanoidSurvivor;
		float Score = 0.0f;
	};

	FEndingScore ScoreEscapeTogether(const FEndingEvaluationContext& C)
	{
		const float AI = static_cast<float>(C.TotalAIInteractions);
		float Score =
			1.3f * High(C.Trust, 0.50f, 0.12f) +
			1.1f * High(C.Kindness, 0.46f, 0.10f) +
			1.1f * High(C.Cooperation, 0.52f, 0.12f) +
			1.3f * Low(C.Dependency, 0.48f, 0.12f) +
			1.5f * High(C.AIStability, 0.76f, 0.08f) +
			1.0f * High(AI, 4.0f, 2.0f) +
			0.7f * Low(C.Suspicion, 0.40f, 0.12f) -
			2.0f * Low(C.Kindness, 0.44f, 0.10f) -
			1.8f * High(C.Dependency, 0.48f, 0.10f) -
			2.0f * Low(C.AIStability, 0.74f, 0.08f);

		// First-run good ending: talked, stayed kind, did not cling, AI stayed stable.
		if (C.TotalAIInteractions >= 4 && C.TotalAIInteractions <= 8
			&& C.Dependency < 0.48f
			&& C.AIStability >= 0.76f
			&& C.Kindness >= 0.46f
			&& C.Trust >= 0.50f)
		{
			Score += 1.6f;
		}

		return { ELoopEndingType::EscapeTogether, Score };
	}

	FEndingScore ScoreColdBetrayal(const FEndingEvaluationContext& C)
	{
		const float AI = static_cast<float>(C.TotalAIInteractions);
		float Score =
			1.1f * High(C.Trust, 0.48f, 0.10f) +
			2.6f * Low(C.Kindness, 0.42f, 0.12f) +
			1.0f * High(C.Cooperation, 0.48f, 0.12f) +
			0.7f * High(C.Dependency, 0.28f, 0.10f) +
			1.0f * High(AI, 4.0f, 2.0f) +
			0.5f * High(C.Trust, 0.58f, 0.10f) -
			2.0f * High(C.Kindness, 0.46f, 0.08f);

		if (C.Kindness <= 0.42f && C.TotalAIInteractions >= 4 && C.Trust >= 0.48f)
		{
			Score += 2.4f;
		}

		return { ELoopEndingType::ColdBetrayal, Score };
	}

	FEndingScore ScoreObedientFool(const FEndingEvaluationContext& C)
	{
		const float AI = static_cast<float>(C.TotalAIInteractions);
		float Score =
			0.9f * High(C.Trust, 0.48f, 0.10f) +
			0.8f * High(C.Kindness, 0.42f, 0.10f) +
			2.4f * High(C.Dependency, 0.44f, 0.10f) +
			1.1f * Low(C.Suspicion, 0.42f, 0.12f) +
			1.2f * High(AI, 6.0f, 2.0f) +
			0.5f * High(C.Cooperation, 0.45f, 0.12f) -
			1.6f * Low(C.Kindness, 0.38f, 0.08f) -
			0.6f * Low(C.Dependency, 0.40f, 0.08f);

		if (C.Dependency >= 0.48f && C.TotalAIInteractions >= 6
			&& C.Kindness >= 0.40f && C.Suspicion <= 0.45f)
		{
			Score += 2.2f;
		}

		return { ELoopEndingType::ObedientFool, Score };
	}

	FEndingScore ScoreMergedMemory(const FEndingEvaluationContext& C)
	{
		const float AI = static_cast<float>(C.TotalAIInteractions);
		const float Advances = static_cast<float>(C.TotalAdvances);
		float Score =
			1.1f * High(C.Kindness, 0.50f, 0.10f) +
			1.1f * High(C.Cooperation, 0.52f, 0.10f) +
			2.8f * Low(C.AIStability, 0.72f, 0.08f) +
			0.8f * High(AI, 5.0f, 2.0f) +
			0.5f * High(Advances, 7.0f, 2.0f) +
			0.4f * High(C.Trust, 0.42f, 0.12f) -
			1.8f * High(C.AIStability, 0.78f, 0.08f) -
			1.0f * High(C.Dependency, 0.58f, 0.08f);

		if (C.AIStability <= 0.76f && C.Kindness >= 0.50f
			&& C.TotalAIInteractions >= 5 && C.Dependency < 0.58f)
		{
			Score += 2.2f;
		}

		return { ELoopEndingType::MergedMemory, Score };
	}

	FEndingScore ScoreTheReplacement(const FEndingEvaluationContext& C)
	{
		const float AI = static_cast<float>(C.TotalAIInteractions);
		float Score =
			0.9f * High(C.Trust, 0.52f, 0.10f) +
			0.9f * High(C.Kindness, 0.52f, 0.10f) +
			0.9f * High(C.Cooperation, 0.55f, 0.10f) +
			1.8f * High(C.Dependency, 0.52f, 0.10f) +
			1.8f * Low(C.AIStability, 0.68f, 0.08f) +
			2.0f * High(AI, 9.0f, 2.0f) -
			2.4f * (1.0f - High(AI, 8.0f, 1.0f)) -
			0.8f * Low(C.Dependency, 0.50f, 0.08f);

		if (C.TotalAIInteractions >= 9 && C.Dependency >= 0.52f
			&& C.AIStability <= 0.70f && C.Kindness >= 0.50f)
		{
			Score += 2.6f;
		}

		return { ELoopEndingType::TheReplacement, Score };
	}

	FEndingScore ScoreParanoidSurvivor(const FEndingEvaluationContext& C)
	{
		const float AI = static_cast<float>(C.TotalAIInteractions);
		const float Score =
			2.0f * Low(C.Trust, 0.46f, 0.12f) +
			2.0f * High(C.Suspicion, 0.42f, 0.12f) +
			1.2f * Low(AI, 4.0f, 2.0f) +
			0.6f * Low(C.Cooperation, 0.48f, 0.10f) -
			1.6f * High(C.Trust, 0.55f, 0.10f) -
			0.8f * High(AI, 5.0f, 2.0f);

		return { ELoopEndingType::ParanoidSurvivor, Score };
	}

	const TCHAR* EndingShortName(ELoopEndingType Type)
	{
		switch (Type)
		{
		case ELoopEndingType::EscapeTogether: return TEXT("Escape");
		case ELoopEndingType::ObedientFool: return TEXT("Obedient");
		case ELoopEndingType::ColdBetrayal: return TEXT("Cold");
		case ELoopEndingType::MergedMemory: return TEXT("Merged");
		case ELoopEndingType::TheReplacement: return TEXT("Replace");
		case ELoopEndingType::TheExit: return TEXT("Exit");
		default: return TEXT("Paranoid");
		}
	}
}

ELoopEndingType FLoopEndingEvaluator::Evaluate(const FEndingEvaluationContext& Context)
{
	// Never (or barely) talked — escaped alone, not as anyone's puppet.
	if (Context.TotalAIInteractions < MinAIInteractionsForRelationshipEndings)
	{
		UE_LOG(LogTemp, Log,
			TEXT("Ending eval: AI=%d < %d -> ParanoidSurvivor"),
			Context.TotalAIInteractions,
			MinAIInteractionsForRelationshipEndings);
		return ELoopEndingType::ParanoidSurvivor;
	}

	const FEndingScore Scores[] = {
		ScoreEscapeTogether(Context),
		ScoreObedientFool(Context),
		ScoreColdBetrayal(Context),
		ScoreMergedMemory(Context),
		ScoreTheReplacement(Context),
		ScoreParanoidSurvivor(Context),
	};

	FEndingScore Best = Scores[0];
	for (const FEndingScore& Candidate : Scores)
	{
		if (Candidate.Score > Best.Score)
		{
			Best = Candidate;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("Ending eval: T=%.2f K=%.2f C=%.2f S=%.2f D=%.2f St=%.2f AI=%d Adv=%d | Escape=%.2f Obedient=%.2f Cold=%.2f Merged=%.2f Replace=%.2f Paranoid=%.2f -> %s"),
		Context.Trust, Context.Kindness, Context.Cooperation, Context.Suspicion,
		Context.Dependency, Context.AIStability,
		Context.TotalAIInteractions, Context.TotalAdvances,
		Scores[0].Score, Scores[1].Score, Scores[2].Score,
		Scores[3].Score, Scores[4].Score, Scores[5].Score,
		EndingShortName(Best.Type));

	return Best.Type;
}
