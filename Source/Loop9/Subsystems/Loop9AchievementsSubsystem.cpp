#include "Subsystems/Loop9AchievementsSubsystem.h"

#include "Misc/ConfigCacheIni.h"
#include "OnlineSubsystem.h"
#include "OnlineStats.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Steam/Loop9SteamUtils.h"

namespace
{
	constexpr int32 StreakTarget = 7;
	constexpr int32 GroundhogResetTarget = 10;
	constexpr int32 HotlineMessageTarget = 15;
	constexpr int32 HalfwayLoopIndex = 5;
	constexpr int32 EndingTypeCount = 6;
	constexpr int32 AnomalyTypeCount = 9;

	const TCHAR* PersistSection = TEXT("/Script/Loop9.Loop9AchievementsSubsystem");
	const TCHAR* SeenEndingsKey = TEXT("SeenEndings");
	const TCHAR* SpottedAnomaliesKey = TEXT("SpottedAnomalies");

	IOnlineAchievementsPtr GetAchievementsInterface()
	{
		if (IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
		{
			return OnlineSubsystem->GetAchievementsInterface();
		}

		return nullptr;
	}

	FUniqueNetIdPtr GetLocalPlayerId()
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (!OnlineSubsystem)
		{
			return nullptr;
		}

		IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface();
		return Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
	}
}

void ULoop9AchievementsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Clock never shipped as a playable anomaly. Strip its old development
	// marker so it cannot inflate ACH_SPOT_ALL progress after removal.
	TArray<FString> Spotted = LoadPersistedList(SpottedAnomaliesKey);
	if (Spotted.Remove(TEXT("ClockAnomaly")) > 0)
	{
		SavePersistedList(SpottedAnomaliesKey, Spotted);
	}

	QueryAchievementsCache();
}

FName ULoop9AchievementsSubsystem::EndingAchievementId(ELoopEndingType EndingType)
{
	switch (EndingType)
	{
	case ELoopEndingType::EscapeTogether: return FName(TEXT("ACH_ENDING_ESCAPE_TOGETHER"));
	case ELoopEndingType::ObedientFool: return FName(TEXT("ACH_ENDING_OBEDIENT_FOOL"));
	case ELoopEndingType::ColdBetrayal: return FName(TEXT("ACH_ENDING_COLD_BETRAYAL"));
	case ELoopEndingType::ParanoidSurvivor: return FName(TEXT("ACH_ENDING_PARANOID_SURVIVOR"));
	case ELoopEndingType::MergedMemory: return FName(TEXT("ACH_ENDING_MERGED_MEMORY"));
	case ELoopEndingType::TheReplacement: return FName(TEXT("ACH_ENDING_THE_REPLACEMENT"));
	default: return NAME_None;
	}
}

FName ULoop9AchievementsSubsystem::SpotAchievementId(const FString& AnomalyTypeLabel)
{
	if (AnomalyTypeLabel == TEXT("HideAnomaly")) { return FName(TEXT("ACH_SPOT_HIDE")); }
	if (AnomalyTypeLabel == TEXT("MoveAnomaly")) { return FName(TEXT("ACH_SPOT_MOVE")); }
	if (AnomalyTypeLabel == TEXT("LightFlickerAnomaly")) { return FName(TEXT("ACH_SPOT_LIGHT")); }
	if (AnomalyTypeLabel == TEXT("AudioAnomaly")) { return FName(TEXT("ACH_SPOT_AUDIO")); }
	if (AnomalyTypeLabel == TEXT("TextAnomaly")) { return FName(TEXT("ACH_SPOT_TEXT")); }
	if (AnomalyTypeLabel == TEXT("DoorLockAnomaly")) { return FName(TEXT("ACH_SPOT_DOORLOCK")); }
	if (AnomalyTypeLabel == TEXT("PursuerAnomaly")) { return FName(TEXT("ACH_SPOT_PURSUER")); }
	if (AnomalyTypeLabel == TEXT("ScaleAnomaly")) { return FName(TEXT("ACH_SPOT_SCALE")); }
	if (AnomalyTypeLabel == TEXT("PhantomMessageAnomaly")) { return FName(TEXT("ACH_SPOT_PHANTOM")); }
	return NAME_None;
}

void ULoop9AchievementsSubsystem::NotifyLoopDecision(bool bWasCorrect, bool bAnomalyExisted, const FString& AnomalyKey, bool bRepeatAnomaly)
{
	if (bWasCorrect)
	{
		CorrectDecisionStreak++;
		if (CorrectDecisionStreak >= StreakTarget)
		{
			UnlockAchievement(FName(TEXT("ACH_STREAK_7")));
		}
		else
		{
			FLoop9SteamUtils::IndicateAchievementProgress(
				FName(TEXT("ACH_STREAK_7")), CorrectDecisionStreak, StreakTarget);
		}
	}
	else
	{
		CorrectDecisionStreak = 0;
	}

	// Wrong decisions (either kind) send the player back to floor 1.
	const bool bDecisionCausedReset = !bWasCorrect;
	if (bDecisionCausedReset)
	{
		ResetsThisRun++;
		UnlockAchievement(FName(TEXT("ACH_FIRST_RESET")));
		if (ResetsThisRun >= GroundhogResetTarget)
		{
			UnlockAchievement(FName(TEXT("ACH_GROUNDHOG")));
		}
		else
		{
			FLoop9SteamUtils::IndicateAchievementProgress(
				FName(TEXT("ACH_GROUNDHOG")), ResetsThisRun, GroundhogResetTarget);
		}
	}

	if (bWasCorrect && bAnomalyExisted)
	{
		RecordSpottedAnomalies(AnomalyKey);

		if (bRepeatAnomaly)
		{
			UnlockAchievement(FName(TEXT("ACH_DEJA_VU")));
		}
	}
}

void ULoop9AchievementsSubsystem::NotifyLoopReached(int32 LoopIndex)
{
	if (LoopIndex >= HalfwayLoopIndex)
	{
		UnlockAchievement(FName(TEXT("ACH_REACH_LOOP_5")));
	}
}

void ULoop9AchievementsSubsystem::NotifyAIMessageSent(int32 TotalInteractions)
{
	UnlockAchievement(FName(TEXT("ACH_FIRST_CALL")));

	if (TotalInteractions >= HotlineMessageTarget)
	{
		UnlockAchievement(FName(TEXT("ACH_HOTLINE")));
	}
	else
	{
		FLoop9SteamUtils::IndicateAchievementProgress(
			FName(TEXT("ACH_HOTLINE")), TotalInteractions, HotlineMessageTarget);
	}
}

void ULoop9AchievementsSubsystem::NotifyRunFinished(ELoopEndingType EndingType, int32 TotalResets, int32 TotalAIInteractions)
{
	UnlockAchievement(FName(TEXT("ACH_FINISH_RUN")));
	UnlockAchievement(EndingAchievementId(EndingType));
	RecordSeenEnding(EndingType);

	if (TotalResets == 0)
	{
		UnlockAchievement(FName(TEXT("ACH_PERFECT_RUN")));
	}

	if (TotalAIInteractions == 0)
	{
		UnlockAchievement(FName(TEXT("ACH_SILENT_RUN")));
	}

	NotifyRunRestarted();
}

void ULoop9AchievementsSubsystem::NotifyRunRestarted()
{
	CorrectDecisionStreak = 0;
	ResetsThisRun = 0;
}

void ULoop9AchievementsSubsystem::RecordSeenEnding(ELoopEndingType EndingType)
{
	const FName EndingId = EndingAchievementId(EndingType);
	if (EndingId.IsNone())
	{
		return;
	}

	TArray<FString> Seen = LoadPersistedList(SeenEndingsKey);
	Seen.AddUnique(EndingId.ToString());
	SavePersistedList(SeenEndingsKey, Seen);

	if (Seen.Num() >= EndingTypeCount)
	{
		UnlockAchievement(FName(TEXT("ACH_ALL_ENDINGS")));
	}
	else
	{
		FLoop9SteamUtils::IndicateAchievementProgress(
			FName(TEXT("ACH_ALL_ENDINGS")), Seen.Num(), EndingTypeCount);
	}
}

void ULoop9AchievementsSubsystem::RecordSpottedAnomalies(const FString& AnomalyKey)
{
	TArray<FString> Labels;
	AnomalyKey.ParseIntoArray(Labels, TEXT("|"), true);

	TArray<FString> Spotted = LoadPersistedList(SpottedAnomaliesKey);
	bool bChanged = Spotted.Remove(TEXT("ClockAnomaly")) > 0;

	for (const FString& Label : Labels)
	{
		const FName SpotId = SpotAchievementId(Label);
		if (SpotId.IsNone())
		{
			continue;
		}

		UnlockAchievement(SpotId);

		if (!Spotted.Contains(Label))
		{
			Spotted.Add(Label);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		SavePersistedList(SpottedAnomaliesKey, Spotted);
	}

	if (Spotted.Num() >= AnomalyTypeCount)
	{
		UnlockAchievement(FName(TEXT("ACH_SPOT_ALL")));
	}
	else if (bChanged)
	{
		FLoop9SteamUtils::IndicateAchievementProgress(
			FName(TEXT("ACH_SPOT_ALL")), Spotted.Num(), AnomalyTypeCount);
	}
}

TArray<FString> ULoop9AchievementsSubsystem::LoadPersistedList(const TCHAR* Key) const
{
	FString Raw;
	GConfig->GetString(PersistSection, Key, Raw, GGameIni);

	TArray<FString> Values;
	Raw.ParseIntoArray(Values, TEXT(","), true);
	return Values;
}

void ULoop9AchievementsSubsystem::SavePersistedList(const TCHAR* Key, const TArray<FString>& Values) const
{
	GConfig->SetString(PersistSection, Key, *FString::Join(Values, TEXT(",")), GGameIni);
	GConfig->Flush(false, GGameIni);
}

void ULoop9AchievementsSubsystem::UnlockAchievement(FName AchievementId)
{
	if (AchievementId.IsNone() || UnlockedThisSession.Contains(AchievementId))
	{
		return;
	}

	if (!GetAchievementsInterface().IsValid() || !GetLocalPlayerId().IsValid())
	{
		UE_LOG(LogTemp, Verbose, TEXT("Achievements: no online subsystem/player, skipping %s."), *AchievementId.ToString());
		return;
	}

	if (!bCacheReady)
	{
		PendingUnlocks.AddUnique(AchievementId);
		QueryAchievementsCache();
		return;
	}

	WriteUnlock(AchievementId);
}

void ULoop9AchievementsSubsystem::QueryAchievementsCache()
{
	if (bCacheReady || bQueryInFlight)
	{
		return;
	}

	IOnlineAchievementsPtr Achievements = GetAchievementsInterface();
	FUniqueNetIdPtr PlayerId = GetLocalPlayerId();
	if (!Achievements.IsValid() || !PlayerId.IsValid())
	{
		return;
	}

	bQueryInFlight = true;

	TWeakObjectPtr<ULoop9AchievementsSubsystem> WeakThis(this);
	Achievements->QueryAchievements(*PlayerId,
		FOnQueryAchievementsCompleteDelegate::CreateLambda(
			[WeakThis](const FUniqueNetId&, const bool bWasSuccessful)
			{
				if (!WeakThis.IsValid())
				{
					return;
				}

				ULoop9AchievementsSubsystem* Self = WeakThis.Get();
				Self->bQueryInFlight = false;
				Self->bCacheReady = bWasSuccessful;

				if (bWasSuccessful)
				{
					Self->FlushPendingUnlocks();
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Achievements: query failed; pending unlocks kept for retry."));
				}
			}));
}

void ULoop9AchievementsSubsystem::FlushPendingUnlocks()
{
	TArray<FName> ToUnlock = MoveTemp(PendingUnlocks);
	PendingUnlocks.Reset();

	for (const FName& AchievementId : ToUnlock)
	{
		if (!UnlockedThisSession.Contains(AchievementId))
		{
			WriteUnlock(AchievementId);
		}
	}
}

void ULoop9AchievementsSubsystem::WriteUnlock(FName AchievementId)
{
	IOnlineAchievementsPtr Achievements = GetAchievementsInterface();
	FUniqueNetIdPtr PlayerId = GetLocalPlayerId();
	if (!Achievements.IsValid() || !PlayerId.IsValid())
	{
		return;
	}

	UnlockedThisSession.Add(AchievementId);

	FOnlineAchievementsWritePtr WriteObject = MakeShareable(new FOnlineAchievementsWrite());
	WriteObject->SetFloatStat(AchievementId.ToString(), 100.0f);

	FOnlineAchievementsWriteRef WriteRef = WriteObject.ToSharedRef();
	Achievements->WriteAchievements(*PlayerId, WriteRef,
		FOnAchievementsWrittenDelegate::CreateLambda(
			[AchievementId](const FUniqueNetId&, bool bWasSuccessful)
			{
				UE_LOG(LogTemp, Log, TEXT("Achievements: unlock %s -> %s"),
					*AchievementId.ToString(), bWasSuccessful ? TEXT("OK") : TEXT("FAILED"));
			}));
}
