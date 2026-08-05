#pragma once

#include "CoreMinimal.h"

namespace Loop9RuntimePolicies
{
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
