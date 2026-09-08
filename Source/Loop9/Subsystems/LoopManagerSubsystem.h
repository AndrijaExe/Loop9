#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop/LoopTypes.h"
#include "Runtime/DragojloCommitmentTracker.h"
#include "LoopManagerSubsystem.generated.h"

class URelationshipSubsystem;
class ULoopEndingPresenterSubsystem;

UCLASS(Blueprintable)
class LOOP9_API ULoopManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, Category = "Loop")
	int32 CurrentLoop = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Loop")
	bool bAnomalyDetected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Loop")
	bool bGameFinished = false;

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void OnElevatorButtonPressed(EButtonType ButtonType);

	/** Creates an owned decision snapshot without changing durable state yet. */
	FLoopElevatorDecision ResolveElevatorDecision(EButtonType ButtonType);

	/**
	 * Applies a previously recorded decision. Cinematic transitions disable the
	 * built-in teleport and may defer ending presentation until doors reopen.
	 */
	bool CommitElevatorDecision(
		const FLoopElevatorDecision& Decision,
		bool bTeleportPlayer = true,
		bool bDeferEndingPresentation = false);

	bool IsElevatorTransitionPending() const { return bElevatorTransitionActive; }
	bool HasDeferredEnding() const { return bDeferredEndingPresentation; }
	bool FinishElevatorTransition(int32 DecisionId);
	bool CancelElevatorDecision(int32 DecisionId);
	void CancelDeferredEnding();
	void CompleteDeferredEnding();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void TeleportPlayerToEntry();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void TeleportPlayerToExit();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	bool HasActiveAnomalies() const;

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void SetAnomalyDetected(bool bDetected);

	UFUNCTION(BlueprintCallable, Category = "Loop")
	bool HasAnomalyBeenDetected() const { return bAnomalyDetected; }

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void AdvanceLoop();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void ResetLoop();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void GenerateAnomalyForNextLoop();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void RegisterAIInteraction(int32 KindnessDelta = 0, int32 SuspicionDelta = 0, int32 DependencyDelta = 0);

	/** Cache live AI_Friend actors so ClearAllAIChats does not scan the world. */
	void RegisterAIFriend(class AAI_Friend* AIFriend);
	void UnregisterAIFriend(class AAI_Friend* AIFriend);

	/** Cache teleport points so FindTeleportPoints does not repeatedly scan. */
	void RegisterTeleportPoint(class ATeleportPoint* Point);
	void UnregisterTeleportPoint(class ATeleportPoint* Point);

	UFUNCTION(BlueprintCallable, Category = "State")
	void ApplyAIDiagnosedKindnessDelta(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "State")
	void ApplyAIDiagnosedSuspicionDelta(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "State")
	void ApplyAIDiagnosedDependencyDelta(int32 Delta);

	/**
	 * Records the structured advice metadata from a validated AI reply and
	 * updates commitment flags from the diagnosed deltas. Does not store chat text.
	 */
	void RecordDragojloAdvice(
		EDragojloAdviceMode Mode,
		EDragojloLiftAdvice LiftAdvice,
		const FString& SuggestedZone,
		const FString& CommitmentId,
		int32 SuspicionDelta,
		int32 DependencyDelta);

	const FDragojloCommitmentState& GetDragojloCommitmentState() const
	{
		return DragojloCommitmentTracker.GetState();
	}

	UFUNCTION(BlueprintCallable, Category = "State")
	void RegisterLoopDecision(bool bWasCorrect, bool bAnomalyExisted, EButtonType ButtonType);

	UFUNCTION(BlueprintCallable, Category = "State")
	void TickAIStabilityDecay();

	void ClearAllAIChats();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void ResetRunState();

	void TriggerEndingSequence();
	ELoopEndingType DetermineEndingType() const;

	/**
	 * 1.1 secret ending. The ground-floor exit door calls this; it only fires
	 * when the door is legitimately reachable — the run is live, the floor is
	 * high enough, and the wall is missing (Hide anomaly active this floor).
	 * Returns false when refused, so the door can stay a plain locked door.
	 */
	UFUNCTION(BlueprintCallable, Category = "Loop|Ending")
	bool TryTriggerSecretExitEnding();

	/** True when TryTriggerSecretExitEnding would be accepted right now. */
	UFUNCTION(BlueprintPure, Category = "Loop|Ending")
	bool IsSecretExitOpen() const;

	/** Lowest floor on which the exit works; below this the door is just a door. */
	UPROPERTY(BlueprintReadWrite, Category = "Loop|Ending")
	int32 SecretExitMinLoop = 4;

	/** Debug (non-shipping): skip the missing-wall requirement for the exit. */
	bool bDebugSecretExitIgnoresWall = false;

	/**
	 * Debug: force relationship + loop state so the next successful advance
	 * (loop 9 -> 10) evaluates to the requested ending. Clears anomalies.
	 * No-op / returns false in shipping builds.
	 */
	bool ApplyEndingTestSetup(ELoopEndingType EndingType);

	UFUNCTION(BlueprintPure, Category = "Loop")
	URelationshipSubsystem* GetRelationship() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetTotalResets() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetTotalAdvances() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetTotalAIInteractions() const;

	UFUNCTION(BlueprintPure, Category = "State")
	float GetTrust() const;

	UFUNCTION(BlueprintPure, Category = "State")
	float GetKindness() const;

	UFUNCTION(BlueprintPure, Category = "State")
	float GetCooperation() const;

	UFUNCTION(BlueprintPure, Category = "State")
	float GetSuspicion() const;

	UFUNCTION(BlueprintPure, Category = "State")
	float GetDependency() const;

	UFUNCTION(BlueprintPure, Category = "State")
	float GetAIStability() const;

private:
	void AdvanceLoopInternal(bool bTeleportPlayer, bool bDeferEndingPresentation);
	void ResetLoopInternal(bool bTeleportPlayer);
	void FindTeleportPoints();
	void PruneStaleTeleportCaches();
	void NotifyAIFriendsLoopChanged();
	void HandleObservationZoneEntered(FName ZoneId);

	UPROPERTY()
	TArray<TWeakObjectPtr<class ATeleportPoint>> EntryPoints;

	UPROPERTY()
	TArray<TWeakObjectPtr<class ATeleportPoint>> ExitPoints;

	UPROPERTY()
	TArray<TWeakObjectPtr<class AAI_Friend>> RegisteredAIFriends;

	TWeakObjectPtr<UWorld> FallbackTeleportScanWorld;

	int32 NextElevatorDecisionId = 0;
	int32 PendingElevatorDecisionId = 0;
	int32 ActiveElevatorDecisionId = 0;
	bool bElevatorTransitionActive = false;
	bool bDeferredEndingPresentation = false;

	FDragojloCommitmentTracker DragojloCommitmentTracker;
};
