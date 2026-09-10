#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "AnomalyManager.generated.h"

class UWorld;

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

	/**
	 * True while at least one registered component of this type is active.
	 * Pursuer stays active for the whole floor visit even after the
	 * manifestation despawns, so this also answers "is he out of the phone".
	 */
	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	bool IsAnomalyTypeActive(ELoopAnomalyType Type) const;

	/** True while a registered component of this type on exactly this actor is active. */
	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	bool IsAnomalyTypeActiveOn(ELoopAnomalyType Type, const AActor* Actor) const;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	int32 GetRegisteredAnomalyCount() const { return RegisteredComponents.Num(); }

	/** Logs all registered anomalies (owner, type, class, active, mat count). */
	UFUNCTION(BlueprintCallable, Category = "Anomaly|Debug")
	void PrintAnomalyStats();

	/**
	 * Logs every material-swap placement that could not be seen if it fired,
	 * and returns how many are broken. Run it from a loaded floor: the mesh and
	 * the normal material are only known once BeginPlay has run.
	 */
	UFUNCTION(BlueprintCallable, Category = "Anomaly|Debug")
	int32 AuditMaterialAnomalies();

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
	/**
	 * Place / kind of the previous floor's anomaly, for the 1.1 stale-floor slip.
	 * Empty when that floor was clean or placeless, so he never points at nothing.
	 */
	FString GetPreviousLoopAnomalyZone() const { return PreviousLoopAnomalyZone; }
	FString GetPreviousLoopAnomalyObjectKind() const { return PreviousLoopAnomalyObjectKind; }

	/**
	 * One authored zone from an inactive, non-Pursuer/non-Phantom component that
	 * differs from every active zone. Empty when no safe decoy exists.
	 */
	FString SelectDecoyZone() const;

	/**
	 * 1.1: answering a ringing desk phone cuts every phone on the floor. Cleared
	 * on the next floor visit and on run reset. Floor-wide by design: the
	 * player learns there is no way back to him this floor.
	 */
	void CutPhoneLineForFloor() { bPhoneLineCutThisFloor = true; }
	bool IsPhoneLineCut() const { return bPhoneLineCutThisFloor; }
	/** Debug only: undo the cut so the ring can be answered again on this floor. */
	void RestorePhoneLine() { bPhoneLineCutThisFloor = false; }

	/** 1.1: a desk phone that has rung once in this run never rings again until the run resets. */
	void MarkPhoneRang(const AActor* Phone) { if (Phone) { PhonesRangThisRun.Add(Phone); } }
	bool HasPhoneRang(const AActor* Phone) const { return Phone && PhonesRangThisRun.Contains(Phone); }
	/** Debug: let every phone ring again without a run reset. */
	void ForgetPhonesRang() { PhonesRangThisRun.Reset(); }

	/** If the map still has no Scale component, attach one to the office printer. */
	void EnsureScaleAnomalyPlacement(UWorld* World);

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<UAnomalyComponentBase>> RegisteredComponents;

	int32 TrackedLoopIndex = INDEX_NONE;
	FString PreviousLoopAnomalyKey;
	FString CurrentLoopAnomalyKey;
	FString CurrentLoopAnomalyContext = TEXT("No active anomaly currently detected.");
	FString CurrentLoopAnomalyZone;
	FString CurrentLoopAnomalyObjectKind;
	FString PreviousLoopAnomalyZone;
	FString PreviousLoopAnomalyObjectKind;
	bool bCurrentLoopAnomalyRepeat = false;
	bool bPhoneLineCutThisFloor = false;
	TSet<TWeakObjectPtr<const AActor>> PhonesRangThisRun;

	void ComputeActiveAnomalySnapshot(
		FString& OutKey,
		FString& OutContext,
		FString& OutZone,
		FString& OutObjectKind) const;
	void CleanupInvalidComponents();

	static bool DoesComponentMatchFilter(const UAnomalyComponentBase* Component, const FString& Filter);
	static bool ForceActivateComponent(UAnomalyComponentBase* Component, int32 MaterialIndex);
};
