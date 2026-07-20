#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop/LoopTypes.h"
#include "LoopManagerSubsystem.generated.h"

class URelationshipSubsystem;
class ULoopEndingPresenterSubsystem;

UCLASS(Blueprintable)
class LOOP9_API ULoopManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
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
	void RegisterAIInteraction();

	UFUNCTION(BlueprintCallable, Category = "State")
	void RegisterPlayerMessage(const FString& Message);

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
	void RegisterLoopDecision(bool bWasCorrect, bool bAnomalyExisted, EButtonType ButtonType);

	UFUNCTION(BlueprintCallable, Category = "State")
	void TickAIStabilityDecay();

	void ClearAllAIChats();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void ResetRunState();

	void TriggerEndingSequence();
	ELoopEndingType DetermineEndingType() const;

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
};
