// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LoopManagerSubsystem.generated.h"

UENUM(BlueprintType)
enum class ELoopEndingType : uint8
{
	EscapeTogether UMETA(DisplayName = "Escape Together"),
	ObedientFool UMETA(DisplayName = "Obedient Fool"),
	ColdBetrayal UMETA(DisplayName = "Cold Betrayal"),
	ParanoidSurvivor UMETA(DisplayName = "Paranoid Survivor"),
	MergedMemory UMETA(DisplayName = "Merged Memory"),
	TheReplacement UMETA(DisplayName = "The Replacement")
};

UENUM(BlueprintType)
enum class EButtonType : uint8
{
	Increment UMETA(DisplayName = "Advance Loop (Next Floor)"),
	Reset UMETA(DisplayName = "Reset Loop (Previous Floor)")
};

UCLASS(Blueprintable)
class LOOP9_API ULoopManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Current loop number
	UPROPERTY(BlueprintReadOnly, Category = "Loop")
	int32 CurrentLoop = 1;

	// TRUE if player detected anomaly (marked by player or script)
	UPROPERTY(BlueprintReadWrite, Category = "Loop")
	bool bAnomalyDetected = false;

	// MAIN FUNCTION - Call this from button press with button type
	UFUNCTION(BlueprintCallable, Category = "Loop")
	void OnElevatorButtonPressed(EButtonType ButtonType);

	// Manual teleport functions (for testing)
	UFUNCTION(BlueprintCallable, Category = "Loop")
	void TeleportPlayerToEntry();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void TeleportPlayerToExit();

	// Check if there are active anomalies in the level
	UFUNCTION(BlueprintCallable, Category = "Loop")
	bool HasActiveAnomalies() const;

	// Mark anomaly as detected by player (call from UI or trigger)
	UFUNCTION(BlueprintCallable, Category = "Loop")
	void SetAnomalyDetected(bool bDetected);

	// Get anomaly detection status
	UFUNCTION(BlueprintCallable, Category = "Loop")
	bool HasAnomalyBeenDetected() const { return bAnomalyDetected; }

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void AdvanceLoop();

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void ResetLoop();

	// Generate random anomaly for next loop iteration
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

	// Clear all AI chat widgets (called on loop reset/advance)
	void ClearAllAIChats();

	// Reset the entire run state to avoid retaining old values between games
	UFUNCTION(BlueprintCallable, Category = "Loop")
	void ResetRunState();

	void TriggerEndingSequence();
	ELoopEndingType DetermineEndingType() const;
	void ShowEndingWidget(ELoopEndingType EndingType);
	void ShowReplacementTerminal();

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalResets = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalAdvances = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalAIInteractions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Trust = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Kindness = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Cooperation = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Suspicion = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Dependency = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float AI_Stability = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Loop")
	bool bGameFinished = false;

private:
	void ClampStateValues();

	void FindTeleportPoints();

	UPROPERTY()
	TArray<TWeakObjectPtr<class ATeleportPoint>> EntryPoints;

	UPROPERTY()
	TArray<TWeakObjectPtr<class ATeleportPoint>> ExitPoints;
};