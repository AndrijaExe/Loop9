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
	void FindTeleportPoints();

	UPROPERTY()
	TArray<TWeakObjectPtr<class ATeleportPoint>> EntryPoints;

	UPROPERTY()
	TArray<TWeakObjectPtr<class ATeleportPoint>> ExitPoints;
};
