#include "Subsystems/Loop9AchievementsSubsystem.h"

#include "Anomaly/AnomalyTypes.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OnlineSubsystem.h"
#include "OnlineStats.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Runtime/Loop9CloudSaveFormat.h"
#include "Runtime/Loop9RuntimePolicies.h"
#include "Steam/Loop9SteamUtils.h"

namespace
{
	constexpr int32 StreakTarget = 7;
	constexpr int32 GroundhogResetTarget = 10;
	constexpr int32 HotlineMessageTarget = 15;
	constexpr int32 HalfwayLoopIndex = 5;
	// Seven since 1.1 (The Exit). ACH_ALL_ENDINGS therefore needs the secret one too.
	constexpr int32 EndingTypeCount = 7;
	// Ten since v1.0.6 (LoopNumber). The 1.1 Watcher has no spot achievement and stays out.
	constexpr int32 SpotAllAnomalyTypeCount = 10;

	// Section and keys live in Loop9CloudSaveFormat, together with the list the
	// file is rewritten from; add a key there, not here.
	const TCHAR* PersistSection = Loop9CloudSaveFormat::Section;
	const TCHAR* SeenEndingsKey = Loop9CloudSaveFormat::SeenEndingsKey;
	const TCHAR* SpottedAnomaliesKey = Loop9CloudSaveFormat::SpottedAnomaliesKey;
	const TCHAR* PendingUnlocksKey = Loop9CloudSaveFormat::PendingUnlocksKey;
	/** Cross-run Dragojlo memory (see FDragojloMemory); rides the same Cloud file. */
	const TCHAR* DragojloMemoryKey = Loop9CloudSaveFormat::DragojloMemoryKey;

	/** Exact Steam Auto-Cloud path: WinAppDataLocal / Loop9/Saved/Config/Windows/Game.ini */
	FString CloudGameIni()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPlatformProcess::UserSettingsDir(),
			TEXT("Loop9"),
			TEXT("Saved"),
			TEXT("Config"),
			TEXT("Windows"),
			TEXT("Game.ini")));
	}

	void LoadCloudFile(FConfigFile& File)
	{
		const FString Ini = CloudGameIni();
		if (IFileManager::Get().FileExists(*Ini))
		{
			File.Read(Ini);
		}
	}

	bool SaveCloudFile(FConfigFile& File)
	{
		File.SetString(PersistSection, Loop9CloudSaveFormat::CloudReadyKey, TEXT("1"));

		// Do not use FConfigFile::Write / WriteToString / GConfig. Those skip or
		// no-op missing inis. Steam Auto-Cloud only needs this file on disk.
		// The whole file is rewritten from Loop9CloudSaveFormat::PersistedKeys();
		// a key missing from that list would be dropped on the next save, which
		// Loop9.Runtime.CloudSave.KeepsEveryKey guards.
		const FString Text = Loop9CloudSaveFormat::BuildIniText(File);

		const FString Ini = CloudGameIni();
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Ini), true);
		const bool bSaved = FFileHelper::SaveStringToFile(
			Text,
			*Ini,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		FLoop9SteamUtils::WriteCloudFile(TEXT("Game.ini"), Text);
		return bSaved;
	}

	IOnlineAchievementsPtr GetAchievementsInterface()
	{
		if (IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
		{
			return OnlineSubsystem->GetAchievementsInterface();
		}

		return nullptr;
	}

	/** Ending achievements against the value SeenEndings stores for each. */
	TMap<FName, FString> EndingAchievementRecord()
	{
		TMap<FName, FString> Record;

		for (const ELoopEndingType EndingType : Loop9RuntimePolicies::AllEndingTypes())
		{
			const FName AchievementId = ULoop9AchievementsSubsystem::EndingAchievementId(EndingType);
			if (!AchievementId.IsNone())
			{
				Record.Add(AchievementId, AchievementId.ToString());
			}
		}

		return Record;
	}

	/** Spot achievements against the anomaly label SpottedAnomalies stores. */
	TMap<FName, FString> SpotAchievementRecord()
	{
		TMap<FName, FString> Record;

		for (const ELoopAnomalyType AnomalyType : AllLoopAnomalyTypes())
		{
			const FString Label = GetLoopAnomalyTypeLabel(AnomalyType).ToString();
			const FName AchievementId = ULoop9AchievementsSubsystem::SpotAchievementId(Label);
			if (!AchievementId.IsNone())
			{
				Record.Add(AchievementId, Label);
			}
		}

		return Record;
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

	for (const FString& PendingId : LoadPersistedList(PendingUnlocksKey))
	{
		const FName AchievementId(*PendingId);
		if (!AchievementId.IsNone())
		{
			PendingUnlocks.AddUnique(AchievementId);
		}
	}
	PersistPendingUnlocks();
	EnsureCloudSaveFile();

	// Steam Auto-Cloud can wipe a brand-new Game.ini during launch sync if the
	// remote side is still empty. Rewrite after that pass, and again on exit.
	if (!CloudRetryTickerHandle.IsValid())
	{
		CloudRetryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &ULoop9AchievementsSubsystem::HandleCloudSaveRetry),
			2.0f);
	}
	if (!EnginePreExitHandle.IsValid())
	{
		EnginePreExitHandle = FCoreDelegates::OnEnginePreExit.AddUObject(
			this, &ULoop9AchievementsSubsystem::HandleEnginePreExit);
	}

	QueryAchievementsCache();
}

void ULoop9AchievementsSubsystem::Deinitialize()
{
	if (PendingRetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PendingRetryTickerHandle);
		PendingRetryTickerHandle.Reset();
	}
	if (CloudRetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(CloudRetryTickerHandle);
		CloudRetryTickerHandle.Reset();
	}
	if (EnginePreExitHandle.IsValid())
	{
		FCoreDelegates::OnEnginePreExit.Remove(EnginePreExitHandle);
		EnginePreExitHandle.Reset();
	}

	PersistPendingUnlocks();
	EnsureCloudSaveFile();
	PendingUnlocks.Reset();
	InFlightUnlocks.Reset();
	Super::Deinitialize();
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
	case ELoopEndingType::TheExit: return FName(TEXT("ACH_ENDING_THE_EXIT"));
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
	if (AnomalyTypeLabel == TEXT("LoopNumberAnomaly")) { return FName(TEXT("ACH_SPOT_LOOPNUMBER")); }
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

	// Spotless Record is the nine-floor route without a wrong call. The Exit
	// leaves from floor four, so three clean lifts must not earn it.
	if (TotalResets == 0 && EndingType != ELoopEndingType::TheExit)
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

TArray<FString> ULoop9AchievementsSubsystem::GetSeenEndingIds() const
{
	return MergeWithSteam(SeenEndingsKey, EndingAchievementRecord());
}

void ULoop9AchievementsSubsystem::RecordSeenEnding(ELoopEndingType EndingType)
{
	const FName EndingId = EndingAchievementId(EndingType);
	if (EndingId.IsNone())
	{
		return;
	}

	TArray<FString> Seen = MergeWithSteam(SeenEndingsKey, EndingAchievementRecord());
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

	TArray<FString> Spotted = MergeWithSteam(SpottedAnomaliesKey, SpotAchievementRecord());
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

	if (Spotted.Num() >= SpotAllAnomalyTypeCount)
	{
		UnlockAchievement(FName(TEXT("ACH_SPOT_ALL")));
	}
	else if (bChanged)
	{
		FLoop9SteamUtils::IndicateAchievementProgress(
			FName(TEXT("ACH_SPOT_ALL")), Spotted.Num(), SpotAllAnomalyTypeCount);
	}
}

FString ULoop9AchievementsSubsystem::LoadPersistedValue(const TCHAR* Key) const
{
	FString Raw;
	FConfigFile CloudFile;
	LoadCloudFile(CloudFile);
	CloudFile.GetString(PersistSection, Key, Raw);
	if (Raw.IsEmpty() && !GGameIni.IsEmpty())
	{
		GConfig->GetString(PersistSection, Key, Raw, GGameIni);
	}
	return Raw;
}

void ULoop9AchievementsSubsystem::SavePersistedValue(const TCHAR* Key, const FString& Value) const
{
	FConfigFile CloudFile;
	LoadCloudFile(CloudFile);
	CloudFile.SetString(PersistSection, Key, *Value);
	CloudFile.SetString(PersistSection, TEXT("CloudReady"), TEXT("1"));
	SaveCloudFile(CloudFile);
	if (!GGameIni.IsEmpty())
	{
		GConfig->SetString(PersistSection, Key, *Value, GGameIni);
	}
}

FString ULoop9AchievementsSubsystem::LoadDragojloMemory() const
{
	return LoadPersistedValue(DragojloMemoryKey);
}

void ULoop9AchievementsSubsystem::SaveDragojloMemory(const FString& Persisted) const
{
	SavePersistedValue(DragojloMemoryKey, Persisted);
}

TArray<FString> ULoop9AchievementsSubsystem::LoadPersistedList(const TCHAR* Key) const
{
	TArray<FString> Values;
	LoadPersistedValue(Key).ParseIntoArray(Values, TEXT(","), true);
	return Values;
}

void ULoop9AchievementsSubsystem::SavePersistedList(const TCHAR* Key, const TArray<FString>& Values) const
{
	SavePersistedValue(Key, FString::Join(Values, TEXT(",")));
}

void ULoop9AchievementsSubsystem::EnsureCloudSaveFile() const
{
	FConfigFile CloudFile;
	LoadCloudFile(CloudFile);
	CloudFile.SetString(PersistSection, TEXT("CloudReady"), TEXT("1"));
	SaveCloudFile(CloudFile);
}

bool ULoop9AchievementsSubsystem::HandleCloudSaveRetry(float)
{
	CloudRetryTickerHandle.Reset();
	EnsureCloudSaveFile();
	return false;
}

void ULoop9AchievementsSubsystem::HandleEnginePreExit()
{
	EnsureCloudSaveFile();
}

TArray<FString> ULoop9AchievementsSubsystem::MergeWithSteam(
	const TCHAR* Key,
	const TMap<FName, FString>& AchievementToValue) const
{
	TArray<FString> Values = LoadPersistedList(Key);

	TArray<FName> Candidates;
	AchievementToValue.GetKeys(Candidates);

	TArray<FString> FromSteam;
	for (const FName& AchievementId : FLoop9SteamUtils::UnlockedAchievements(Candidates))
	{
		if (const FString* Value = AchievementToValue.Find(AchievementId))
		{
			FromSteam.Add(*Value);
		}
	}

	if (Loop9RuntimePolicies::AddMissingValues(Values, FromSteam))
	{
		SavePersistedList(Key, Values);
	}

	return Values;
}

void ULoop9AchievementsSubsystem::UnlockAchievement(FName AchievementId)
{
	if (AchievementId.IsNone()
		|| UnlockedThisSession.Contains(AchievementId)
		|| InFlightUnlocks.Contains(AchievementId))
	{
		return;
	}

	// Straight through Steamworks first. That path needs neither the online
	// subsystem's cached achievement list nor a resolved identity, and the
	// subsystem refuses every write unless every achievement is also listed
	// under [OnlineSubsystemSteam] in DefaultEngine.ini.
	if (FLoop9SteamUtils::UnlockAchievement(AchievementId))
	{
		UnlockedThisSession.Add(AchievementId);
		PendingUnlocks.Remove(AchievementId);
		PersistPendingUnlocks();
		UE_LOG(LogTemp, Log, TEXT("Achievements: unlock %s -> OK"), *AchievementId.ToString());

		return;
	}

	QueuePendingUnlock(AchievementId);

	if (!GetAchievementsInterface().IsValid() || !GetLocalPlayerId().IsValid())
	{
		SchedulePendingRetry();
		UE_LOG(LogTemp, Verbose, TEXT("Achievements: no online subsystem/player; queued %s."), *AchievementId.ToString());
		return;
	}

	if (!bCacheReady)
	{
		QueryAchievementsCache();
		return;
	}

	FlushPendingUnlocks();
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
		if (PendingUnlocks.Num() > 0)
		{
			SchedulePendingRetry();
		}
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
					Self->PendingRetryDelaySeconds = 2.0f;
					Self->FlushPendingUnlocks();
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Achievements: query failed; pending unlocks kept for retry."));
					if (Self->PendingUnlocks.Num() > 0)
					{
						Self->SchedulePendingRetry();
					}
				}
			}));
}

void ULoop9AchievementsSubsystem::FlushPendingUnlocks()
{
	TArray<FName> ToUnlock = MoveTemp(PendingUnlocks);
	PendingUnlocks.Reset();

	for (const FName& AchievementId : ToUnlock)
	{
		if (!UnlockedThisSession.Contains(AchievementId) && !InFlightUnlocks.Contains(AchievementId))
		{
			if (!WriteUnlock(AchievementId))
			{
				bCacheReady = false;
				PendingUnlocks.AddUnique(AchievementId);
			}
		}
	}

	PersistPendingUnlocks();

	if (PendingUnlocks.Num() > 0)
	{
		SchedulePendingRetry();
	}
}

bool ULoop9AchievementsSubsystem::WriteUnlock(FName AchievementId)
{
	// Retries land here, so give Steamworks another direct turn before falling
	// back to the online subsystem.
	if (FLoop9SteamUtils::UnlockAchievement(AchievementId))
	{
		UnlockedThisSession.Add(AchievementId);
		UE_LOG(LogTemp, Log, TEXT("Achievements: unlock %s -> OK"), *AchievementId.ToString());

		return true;
	}

	IOnlineAchievementsPtr Achievements = GetAchievementsInterface();
	FUniqueNetIdPtr PlayerId = GetLocalPlayerId();
	if (!Achievements.IsValid() || !PlayerId.IsValid())
	{
		return false;
	}

	InFlightUnlocks.Add(AchievementId);

	FOnlineAchievementsWritePtr WriteObject = MakeShareable(new FOnlineAchievementsWrite());
	WriteObject->SetFloatStat(AchievementId.ToString(), 100.0f);

	TWeakObjectPtr<ULoop9AchievementsSubsystem> WeakThis(this);
	FOnlineAchievementsWriteRef WriteRef = WriteObject.ToSharedRef();
	Achievements->WriteAchievements(*PlayerId, WriteRef,
		FOnAchievementsWrittenDelegate::CreateLambda(
			[WeakThis, AchievementId](const FUniqueNetId&, bool bWasSuccessful)
			{
				UE_LOG(LogTemp, Log, TEXT("Achievements: unlock %s -> %s"),
					*AchievementId.ToString(), bWasSuccessful ? TEXT("OK") : TEXT("FAILED"));

				if (!WeakThis.IsValid())
				{
					return;
				}

				ULoop9AchievementsSubsystem* Self = WeakThis.Get();
				Self->InFlightUnlocks.Remove(AchievementId);
				if (bWasSuccessful)
				{
					Self->UnlockedThisSession.Add(AchievementId);
					Self->PendingRetryDelaySeconds = 2.0f;
					Self->PendingUnlocks.Remove(AchievementId);
					Self->PersistPendingUnlocks();
					return;
				}

				// The online API exposes only success/failure here, not a permanent
				// error category. Keep the unlock queued with capped backoff.
				Self->PendingUnlocks.AddUnique(AchievementId);
				Self->PersistPendingUnlocks();
				Self->SchedulePendingRetry();
			}));

	return true;
}

void ULoop9AchievementsSubsystem::QueuePendingUnlock(FName AchievementId)
{
	if (!AchievementId.IsNone() && !PendingUnlocks.Contains(AchievementId))
	{
		PendingUnlocks.Add(AchievementId);
		PersistPendingUnlocks();
	}
}

void ULoop9AchievementsSubsystem::PersistPendingUnlocks() const
{
	SavePersistedList(
		PendingUnlocksKey,
		Loop9RuntimePolicies::MergePendingAchievementIds(PendingUnlocks, InFlightUnlocks));
}

void ULoop9AchievementsSubsystem::SchedulePendingRetry()
{
	if (!PendingRetryTickerHandle.IsValid())
	{
		PendingRetryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(
				this, &ULoop9AchievementsSubsystem::HandlePendingRetry),
			PendingRetryDelaySeconds);
		PendingRetryDelaySeconds = FMath::Min(PendingRetryDelaySeconds * 2.0f, 30.0f);
	}
}

bool ULoop9AchievementsSubsystem::HandlePendingRetry(float)
{
	PendingRetryTickerHandle.Reset();

	// The direct Steam write inside WriteUnlock can succeed even when the
	// subsystem cache never becomes ready, so it always gets a turn.
	FlushPendingUnlocks();

	if (!bCacheReady && PendingUnlocks.Num() > 0)
	{
		QueryAchievementsCache();
	}

	return false;
}
