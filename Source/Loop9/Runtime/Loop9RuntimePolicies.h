#pragma once

#include "CoreMinimal.h"
#include "Loop/LoopTypes.h"

namespace Loop9RuntimePolicies
{
	inline int32 ClampStateDelta(int32 Delta)
	{
		return FMath::Clamp(Delta, -1, 1);
	}

	inline int32 MergeKindnessDelta(int32 Left, int32 Right)
	{
		if (Left < 0 || Right < 0)
		{
			return -1;
		}
		if (Left > 0 || Right > 0)
		{
			return 1;
		}
		return 0;
	}

	inline int32 MergeSuspicionDelta(int32 Left, int32 Right)
	{
		if (Left > 0 || Right > 0)
		{
			return 1;
		}
		if (Left < 0 || Right < 0)
		{
			return -1;
		}
		return 0;
	}

	inline int32 MergeDependencyDelta(int32 Left, int32 Right)
	{
		if (Left > 0 || Right > 0)
		{
			return 1;
		}
		if (Left < 0 || Right < 0)
		{
			return -1;
		}
		return 0;
	}

	inline ERunEventTone ToneFromStateDeltas(int32 KindnessDelta, int32 SuspicionDelta)
	{
		if (KindnessDelta < 0)
		{
			return ERunEventTone::Hostile;
		}
		if (SuspicionDelta > 0)
		{
			return ERunEventTone::Suspicious;
		}
		if (KindnessDelta > 0)
		{
			return ERunEventTone::Friendly;
		}
		return ERunEventTone::Neutral;
	}

	inline void FinalizeRunEvent(FRunEvent& Event)
	{
		Event.KindnessDelta = ClampStateDelta(Event.KindnessDelta);
		Event.SuspicionDelta = ClampStateDelta(Event.SuspicionDelta);
		Event.DependencyDelta = ClampStateDelta(Event.DependencyDelta);
		Event.Tone = Event.Type == ERunEventType::Call
			? ToneFromStateDeltas(Event.KindnessDelta, Event.SuspicionDelta)
			: ERunEventTone::Neutral;
	}

	inline void AppendRunEvent(TArray<FRunEvent>& Events, FRunEvent Event)
	{
		if (Event.Count < 1)
		{
			Event.Count = 1;
		}
		FinalizeRunEvent(Event);

		if (Events.Num() > 0 && Event.Type == ERunEventType::Call)
		{
			FRunEvent& Last = Events.Last();
			if (Last.Type == ERunEventType::Call && Last.LoopIndex == Event.LoopIndex)
			{
				Last.Count += Event.Count;
				Last.KindnessDelta = MergeKindnessDelta(Last.KindnessDelta, Event.KindnessDelta);
				Last.SuspicionDelta = MergeSuspicionDelta(Last.SuspicionDelta, Event.SuspicionDelta);
				Last.DependencyDelta = MergeDependencyDelta(Last.DependencyDelta, Event.DependencyDelta);
				FinalizeRunEvent(Last);
				return;
			}
		}

		Events.Add(MoveTemp(Event));
	}

	/** One active anomaly's player-facing detail, ranked by the manager's type order. */
	struct FAnomalyDetailCandidate
	{
		int32 TypePriority = INDEX_NONE;
		FString Zone;
		FString ObjectKind;

		bool IsSet() const { return TypePriority != INDEX_NONE; }
	};

	/**
	 * Picks which active anomaly gets described to the AI. Several can run on one
	 * floor, so the lowest type priority wins and a given world state always
	 * produces the same prompt. Candidates with nothing authored are skipped, so
	 * an unfilled component cannot mask a filled one.
	 */
	inline FAnomalyDetailCandidate SelectAnomalyDetail(const TArray<FAnomalyDetailCandidate>& Candidates)
	{
		FAnomalyDetailCandidate Chosen;

		for (const FAnomalyDetailCandidate& Candidate : Candidates)
		{
			if (!Candidate.IsSet() || (Candidate.Zone.IsEmpty() && Candidate.ObjectKind.IsEmpty()))
			{
				continue;
			}

			if (!Chosen.IsSet() || Candidate.TypePriority < Chosen.TypePriority)
			{
				Chosen = Candidate;
			}
		}

		return Chosen;
	}

	/**
	 * Picks one authored decoy place from inactive components. The zone must
	 * differ from every currently active zone so a planted wrong hint still
	 * points at a real part of the map the player can walk to.
	 */
	inline FString SelectDecoyZone(
		const TArray<FString>& InactiveAuthoredZones,
		const TArray<FString>& ActiveZones)
	{
		TArray<FString> Candidates;
		for (const FString& Zone : InactiveAuthoredZones)
		{
			const FString Trimmed = Zone.TrimStartAndEnd();
			if (Trimmed.IsEmpty())
			{
				continue;
			}

			bool bConflictsWithActive = false;
			for (const FString& Active : ActiveZones)
			{
				if (Active.Equals(Trimmed, ESearchCase::IgnoreCase))
				{
					bConflictsWithActive = true;
					break;
				}
			}

			if (!bConflictsWithActive && !Candidates.ContainsByPredicate([&Trimmed](const FString& Existing)
			{
				return Existing.Equals(Trimmed, ESearchCase::IgnoreCase);
			}))
			{
				Candidates.Add(Trimmed);
			}
		}

		if (Candidates.Num() == 0)
		{
			return FString();
		}

		Candidates.Sort();
		return Candidates[0];
	}

	inline TArray<ELoopEndingType> AllEndingTypes()
	{
		return {
			ELoopEndingType::EscapeTogether,
			ELoopEndingType::ObedientFool,
			ELoopEndingType::ColdBetrayal,
			ELoopEndingType::ParanoidSurvivor,
			ELoopEndingType::MergedMemory,
			ELoopEndingType::TheReplacement,
		};
	}

	inline bool IsEndingUnlocked(const TArray<FString>& SeenAchievementIds, const FString& AchievementId)
	{
		return !AchievementId.IsEmpty() && SeenAchievementIds.Contains(AchievementId);
	}

	/**
	 * Folds Additions into Values, keeping order and skipping duplicates and
	 * empties. Returns true when anything was actually added, so the caller
	 * knows whether the merged list is worth writing back to disk.
	 */
	inline bool AddMissingValues(TArray<FString>& Values, const TArray<FString>& Additions)
	{
		bool bChanged = false;

		for (const FString& Addition : Additions)
		{
			if (!Addition.IsEmpty() && !Values.Contains(Addition))
			{
				Values.Add(Addition);
				bChanged = true;
			}
		}

		return bChanged;
	}

	inline bool ShouldKeepSprintTimerActive(
		bool bSprinting,
		bool bRecovering,
		float CurrentStamina,
		float MaxStamina)
	{
		return bSprinting || bRecovering || CurrentStamina < MaxStamina;
	}

	inline TArray<FString> MergePendingAchievementIds(
		const TArray<FName>& PendingUnlocks,
		const TSet<FName>& InFlightUnlocks)
	{
		TArray<FString> Result;
		Result.Reserve(PendingUnlocks.Num() + InFlightUnlocks.Num());

		for (const FName& AchievementId : PendingUnlocks)
		{
			if (!AchievementId.IsNone())
			{
				Result.AddUnique(AchievementId.ToString());
			}
		}
		for (const FName& AchievementId : InFlightUnlocks)
		{
			if (!AchievementId.IsNone())
			{
				Result.AddUnique(AchievementId.ToString());
			}
		}

		return Result;
	}
}
