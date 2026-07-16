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

	UMeshComponent* ResolveTargetMesh() const;
};
