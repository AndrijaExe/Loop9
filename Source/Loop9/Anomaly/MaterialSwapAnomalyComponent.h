#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "MaterialSwapAnomalyComponent.generated.h"

class UMaterialInterface;
class UMeshComponent;

/**
 * "Spot the difference" anomaly: a mesh on the owning actor shows a different
 * material while the anomaly is active — e.g. a newspaper whose headline
 * changed, a poster with wrong letters, a photo with something missing.
 *
 * Counts as the existing Text anomaly type, so no backend/achievement
 * changes are needed. Works great combined with UInspectableComponent:
 * the inspection view copies the currently applied materials, so the player
 * can pick the item up and read the "wrong" version up close.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UMaterialSwapAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UMaterialSwapAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Text; }

	/**
	 * Materials shown while the anomaly is active. One is picked at random
	 * on each activation, so multiple "wrong" variants stay fresh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Swap Anomaly")
	TArray<TObjectPtr<UMaterialInterface>> AnomalyMaterials;

	/** Material slot on the mesh to swap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Swap Anomaly", meta = (ClampMin = "0"))
	int32 MaterialSlot = 0;

	/**
	 * Name of the mesh component to swap on. Leave None to use the owner's
	 * first mesh component (static or skeletal).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Swap Anomaly")
	FName TargetComponentName = NAME_None;

	/**
	 * Debug/testing only: applies a specific variant without deactivating an
	 * already active anomaly. Returns false without changing the current state
	 * when the variant or target mesh is invalid.
	 */
	bool ForceMaterialVariant(int32 Index);

	/**
	 * Empty when this placement will actually read on screen, otherwise the
	 * reason it will not. Call it in-game: the mesh and the normal material are
	 * only resolved from BeginPlay onwards.
	 *
	 * A swap that activates but changes nothing is the worst outcome available
	 * here. The floor then swears it holds an anomaly, the player searches,
	 * finds every object exactly as it was, correctly concludes the floor is
	 * clean, takes the dark elevator and is punished for being right.
	 */
	FString DescribeConfigurationProblem() const;

protected:
	virtual void BeginPlay() override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> TargetMesh;

	/** Material that was in the slot before the anomaly was applied. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> NormalMaterial;

	/** Explicit capture flag — NormalMaterial may legitimately be null (empty slot). */
	bool bNormalMaterialCaptured = false;

	int32 ForcedMaterialIndex = INDEX_NONE;

	UMeshComponent* ResolveTargetMesh() const;

	/** The material the player treats as normal, whether or not it is applied right now. */
	UMaterialInterface* ResolveBaselineMaterial() const;

	/**
	 * Indices that are non-null and actually differ from the baseline, i.e. the
	 * variants that would be visible if applied.
	 */
	TArray<int32> CollectVisibleVariantIndices() const;
};
