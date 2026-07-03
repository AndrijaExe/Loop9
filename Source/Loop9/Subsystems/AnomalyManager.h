// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AnomalyManager.generated.h"

/**
 * 
 */
UCLASS()
class LOOP9_API UAnomalyManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void RegisterAnomaly(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void UnregisterAnomaly(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void TriggerRandomAnomalies(int32 Count = 3, float MinProbability = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	bool ForceActivateAnyAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ResetAllAnomalies();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	int32 GetActiveAnomalyCount() const;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	int32 GetRegisteredAnomalyCount() const { return RegisteredAnomalies.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void PrintAnomalyStats();

	void UpdateLoopAnomalyTracking(int32 LoopIndex);
	FString GetCurrentLoopAnomalyContext() const { return CurrentLoopAnomalyContext; }
	FString GetCurrentLoopAnomalyKey() const { return CurrentLoopAnomalyKey; }
	bool IsCurrentLoopAnomalyRepeat() const { return bCurrentLoopAnomalyRepeat; }

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> RegisteredAnomalies;

	int32 TrackedLoopIndex = INDEX_NONE;
	FString PreviousLoopAnomalyKey;
	FString CurrentLoopAnomalyKey;
	FString CurrentLoopAnomalyContext = TEXT("No active anomaly currently detected.");
	bool bCurrentLoopAnomalyRepeat = false;

	void ComputeActiveAnomalySnapshot(FString& OutKey, FString& OutContext) const;
};
