#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "AnomalyManager.generated.h"

UCLASS()
class LOOP9_API UAnomalyManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void RegisterAnomalyComponent(UAnomalyComponentBase* Component);

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void UnregisterAnomalyComponent(UAnomalyComponentBase* Component);

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void TriggerRandomAnomalies(int32 Count = 3, float MinProbability = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	bool ForceActivateAnyAnomaly();

	/** Force-activate a random inactive anomaly of the given type (ignores probability). */
	UFUNCTION(BlueprintCallable, Category = "Anomaly|Debug")
	bool ForceActivateByType(ELoopAnomalyType Type);

	/**
	 * Force-activate by filter string. Matches (case-insensitive) against:
	 * anomaly type label (Text, Move, ...), component class name (MaterialSwap, ...),
	 * or owning actor name/label. Optional MaterialIndex forces MaterialSwap variant.
	 */
	UFUNCTION(BlueprintCallable, Category = "Anomaly|Debug")
	bool ForceActivateByFilter(const FString& Filter, int32 MaterialIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ResetAllAnomalies();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	int32 GetActiveAnomalyCount() const;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	int32 GetRegisteredAnomalyCount() const { return RegisteredComponents.Num(); }

	/** Logs all registered anomalies (owner, type, class, active, mat count). */
	UFUNCTION(BlueprintCallable, Category = "Anomaly|Debug")
	void PrintAnomalyStats();

	void UpdateLoopAnomalyTracking(int32 LoopIndex);
	FString GetCurrentLoopAnomalyContext() const { return CurrentLoopAnomalyContext; }
	FString GetCurrentLoopAnomalyKey() const { return CurrentLoopAnomalyKey; }
	bool IsCurrentLoopAnomalyRepeat() const { return bCurrentLoopAnomalyRepeat; }

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<UAnomalyComponentBase>> RegisteredComponents;

	int32 TrackedLoopIndex = INDEX_NONE;
	FString PreviousLoopAnomalyKey;
	FString CurrentLoopAnomalyKey;
	FString CurrentLoopAnomalyContext = TEXT("No active anomaly currently detected.");
	bool bCurrentLoopAnomalyRepeat = false;

	void ComputeActiveAnomalySnapshot(FString& OutKey, FString& OutContext) const;
	void CleanupInvalidComponents();

	static bool DoesComponentMatchFilter(const UAnomalyComponentBase* Component, const FString& Filter);
	static void ForceActivateComponent(UAnomalyComponentBase* Component, int32 MaterialIndex);
};
