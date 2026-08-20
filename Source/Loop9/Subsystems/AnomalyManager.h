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

	/**
	 * Force-activate by filter string. Type labels use an exact case-insensitive
	 * match; class and actor names allow partial matches of at least 3 characters.
	 * Optional MaterialIndex forces a MaterialSwap variant.
	 */
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
	/** Invalidates the cached snapshot when a new floor visit begins. */
	void BeginLoopVisit();
	/** Clears both current and previous visit tracking for a new run. */
	void ResetRunTracking();
	FString GetCurrentLoopAnomalyContext() const { return CurrentLoopAnomalyContext; }
	FString GetCurrentLoopAnomalyKey() const { return CurrentLoopAnomalyKey; }
	bool IsCurrentLoopAnomalyRepeat() const { return bCurrentLoopAnomalyRepeat; }
	/** Coarse place of the anomaly the AI may point at; empty when unauthored or placeless. */
	FString GetCurrentLoopAnomalyZone() const { return CurrentLoopAnomalyZone; }
	/** Category of the affected object; the AI never receives the actor name. */
	FString GetCurrentLoopAnomalyObjectKind() const { return CurrentLoopAnomalyObjectKind; }

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<UAnomalyComponentBase>> RegisteredComponents;

	int32 TrackedLoopIndex = INDEX_NONE;
	FString PreviousLoopAnomalyKey;
	FString CurrentLoopAnomalyKey;
	FString CurrentLoopAnomalyContext = TEXT("No active anomaly currently detected.");
	FString CurrentLoopAnomalyZone;
	FString CurrentLoopAnomalyObjectKind;
	bool bCurrentLoopAnomalyRepeat = false;

	void ComputeActiveAnomalySnapshot(
		FString& OutKey,
		FString& OutContext,
		FString& OutZone,
		FString& OutObjectKind) const;
	void CleanupInvalidComponents();

	static bool DoesComponentMatchFilter(const UAnomalyComponentBase* Component, const FString& Filter);
	static bool ForceActivateComponent(UAnomalyComponentBase* Component, int32 MaterialIndex);
};
