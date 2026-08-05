#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop/LoopTypes.h"
#include "Loop9AchievementsSubsystem.generated.h"

/**
 * Thin wrapper around the Online Subsystem achievements interface (Steam in
 * shipping builds) plus the game's achievement rules. Unlocks are
 * fire-and-forget: without Steam this quietly does nothing, so dev builds and
 * other platforms are unaffected.
 *
 * Achievement IDs must match the "API Name" of achievements defined in
 * Steamworks (App Admin -> Stats & Achievements). The full list with display
 * names, descriptions and unlock conditions lives in STEAM_ACHIEVEMENTS.md.
 *
 * Cross-run progress (seen endings, spotted anomaly types) is persisted in
 * Game.ini so meta-achievements survive restarts even if Steam is offline.
 */
UCLASS()
class LOOP9_API ULoop9AchievementsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Unlocks a single achievement by its Steamworks API name (also callable from Blueprint for one-off triggers). */
	UFUNCTION(BlueprintCallable, Category = "Achievements")
	void UnlockAchievement(FName AchievementId);

	/** Elevator decision was made. AnomalyKey is the AnomalyManager snapshot key ("none" or "TypeA|TypeB"). */
	void NotifyLoopDecision(bool bWasCorrect, bool bAnomalyExisted, const FString& AnomalyKey, bool bRepeatAnomaly);

	/** Loop counter advanced to LoopIndex. */
	void NotifyLoopReached(int32 LoopIndex);

	/** Player sent a message to the AI; TotalInteractions is the run total after this message. */
	void NotifyAIMessageSent(int32 TotalInteractions);

	/** Run ended with the given ending and final run stats. */
	void NotifyRunFinished(ELoopEndingType EndingType, int32 TotalResets, int32 TotalAIInteractions);

	/** A fresh run started (new game from menu). */
	void NotifyRunRestarted();

	/** Steamworks API name for an ending achievement. */
	static FName EndingAchievementId(ELoopEndingType EndingType);

	/** Steamworks API name for spotting a given anomaly type label (e.g. "PursuerAnomaly"). */
	static FName SpotAchievementId(const FString& AnomalyTypeLabel);

private:
	void QueryAchievementsCache();
	void FlushPendingUnlocks();
	bool WriteUnlock(FName AchievementId);
	void QueuePendingUnlock(FName AchievementId);
	void PersistPendingUnlocks() const;
	void SchedulePendingRetry();
	bool HandlePendingRetry(float DeltaSeconds);

	void RecordSeenEnding(ELoopEndingType EndingType);
	void RecordSpottedAnomalies(const FString& AnomalyKey);
	TArray<FString> LoadPersistedList(const TCHAR* Key) const;
	void SavePersistedList(const TCHAR* Key, const TArray<FString>& Values) const;

	// Online plumbing
	bool bCacheReady = false;
	bool bQueryInFlight = false;
	float PendingRetryDelaySeconds = 2.0f;
	TArray<FName> PendingUnlocks;
	TSet<FName> InFlightUnlocks;
	TSet<FName> UnlockedThisSession;
	FTSTicker::FDelegateHandle PendingRetryTickerHandle;

	// Per-run tracking (reset on run restart / finish)
	int32 CorrectDecisionStreak = 0;
	int32 ResetsThisRun = 0;
};
