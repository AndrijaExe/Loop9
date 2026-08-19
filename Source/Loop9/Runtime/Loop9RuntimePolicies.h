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
