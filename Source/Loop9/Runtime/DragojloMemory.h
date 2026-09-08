#pragma once

#include "CoreMinimal.h"
#include "Loop/LoopTypes.h"

/**
 * What Dragojlo remembers about this player across runs. Deliberately tiny:
 * a handful of counters and two coarse labels. It only colours the tone of his
 * first replies in a new run and is never read by the ending evaluator,
 * AdvicePolicy, achievements or telemetry.
 *
 * Persisted as one compact `key=value;...` string next to SeenEndings, so it
 * rides the same Game.ini / Steam Cloud file. Never stores chat text.
 */
struct LOOP9_API FDragojloMemory
{
	/** How many runs reached an ending on this install. */
	int32 RunsFinished = 0;

	/** Ending of the most recent finished run; only meaningful when bHasLastEnding. */
	ELoopEndingType LastEnding = ELoopEndingType::ParanoidSurvivor;
	bool bHasLastEnding = false;

	/** Successful phone replies in the most recent finished run. */
	int32 LastRunCalls = 0;

	/** Final kindness of the most recent run, bucketed: -1 cold, 0 neutral, 1 warm. */
	int32 LastRunTone = 0;

	/** Planted locations + wrong lifts across every run. */
	int32 LiesTold = 0;

	/** How many runs ended with the player having exposed one of those lies. */
	int32 CaughtLying = 0;

	/** Runs in which the player followed his last lift call at least once. */
	int32 RunsFollowingHim = 0;

	bool IsEmpty() const { return RunsFinished <= 0; }

	/** Folds one finished run into the memory. */
	void RecordRunFinished(
		ELoopEndingType EndingType,
		int32 TotalAIInteractions,
		float FinalKindness,
		const FDragojloCommitmentState& Commitment);

	/** `runs=2;last=cold_betrayal;calls=7;tone=-1;lies=1;caught=1;followed=1`. */
	FString ToPersistedString() const;
	static FDragojloMemory FromPersistedString(const FString& Raw);

	/** Backend wire label for an ending (`snake_case`, stable across localizations). */
	static FString EndingWireLabel(ELoopEndingType EndingType);
	static bool TryParseEndingWireLabel(const FString& Label, ELoopEndingType& OutEnding);

	/**
	 * Tone should only shape the first replies of a fresh run; after that the
	 * live relationship speaks for itself.
	 */
	static constexpr int32 MaxInteractionsForRecall = 3;
	static bool ShouldSendToBackend(const FDragojloMemory& Memory, int32 InteractionsThisRun)
	{
		return !Memory.IsEmpty() && InteractionsThisRun < MaxInteractionsForRecall;
	}

	static int32 ToneBucket(float Kindness)
	{
		if (Kindness >= 0.60f)
		{
			return 1;
		}
		return Kindness <= 0.42f ? -1 : 0;
	}
};
