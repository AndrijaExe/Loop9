#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InspectableComponent.generated.h"

class APlayerController;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLoop9InspectionEvent);

/**
 * Makes the owning actor inspectable Resident Evil style: the game pauses,
 * the view switches to an isolated black room where a lit copy of the item's
 * mesh is shown and the player can rotate it with the mouse (or the right
 * gamepad stick).
 *
 * Add this component to an actor with a static mesh and call StartInspection()
 * from its interaction code (see AInspectableItem for a ready-made actor).
 */
UCLASS(ClassGroup = (Loop9), meta = (BlueprintSpawnableComponent))
class LOOP9_API UInspectableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInspectableComponent();

	/** Optional item name (reserved for a future caption UI; not shown yet). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection")
	FText DisplayName;

	/**
	 * Prompt shown when the player looks at the owner. Used when the owner
	 * does not implement ILoop9Interactable itself — just add this component
	 * to any actor and it becomes inspectable with this prompt.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection")
	FText PromptText = NSLOCTEXT("Loop9Interaction", "Inspect", "Examine");

	/** Mesh shown while inspecting. If unset, the owner's first StaticMeshComponent is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection")
	TObjectPtr<UStaticMesh> MeshOverride;

	/**
	 * Optional component name when the owner has multiple static meshes.
	 * If unset, the first StaticMeshComponent is used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection")
	FName TargetMeshComponentName = NAME_None;

	/**
	 * With MeshOverride set, copy the target component's current materials.
	 * Keep enabled for runtime material anomalies; disable when the override
	 * mesh has incompatible material slots.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection")
	bool bCopyOwnerMaterialsWithMeshOverride = true;

	/**
	 * Optional per-slot materials used only during inspection. Use opaque
	 * variants here when a world material is translucent/dithered or depends
	 * on world position. Non-null entries override copied source materials.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection")
	TArray<TObjectPtr<UMaterialInterface>> InspectionMaterialOverrides;

	/** Extra rotation applied when the inspection opens, so the item starts on its "nice" side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection")
	FRotator InitialRotationOffset = FRotator::ZeroRotator;

	/** Degrees of rotation per mouse unit while dragging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float RotationSpeed = 0.6f;

	/** The mesh is auto-scaled so its bounding-sphere radius matches this many cm on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection", meta = (ClampMin = "2.0", ClampMax = "60.0"))
	float TargetRadiusCm = 16.0f;

	/** Distance from the camera at which the item is displayed (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection", meta = (ClampMin = "20.0", ClampMax = "150.0"))
	float DistanceCm = 45.0f;

	/** Fired when the inspection view opens. */
	UPROPERTY(BlueprintAssignable, Category = "Inspection")
	FLoop9InspectionEvent OnInspectionStarted;

	/** Fired when the player closes the inspection view. */
	UPROPERTY(BlueprintAssignable, Category = "Inspection")
	FLoop9InspectionEvent OnInspectionEnded;

	/**
	 * Opens the inspection view. Returns false if it could not start
	 * (no mesh, game already paused, or another inspection is active).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inspection")
	bool StartInspection(APlayerController* InteractingController);

	/** Called by the stage actor when the view closes. Not for external use. */
	void NotifyInspectionEnded();

	/** Resolves the mesh (and material overrides, if taken from a component) to display. */
	UStaticMesh* ResolveMesh(TArray<UMaterialInterface*>& OutMaterialOverrides) const;

private:
	UStaticMeshComponent* ResolveTargetMeshComponent() const;
};
