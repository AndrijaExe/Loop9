#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Scene.h"
#include "InspectionStageActor.generated.h"

class APlayerController;
class UCameraComponent;
class UInspectableComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * Internal actor that runs a single item-inspection session:
 * pauses the game, blurs the background with camera depth of field,
 * shows a copy of the item's mesh in front of the camera and lets the
 * player rotate it with the mouse / right gamepad stick.
 *
 * Spawned by UInspectableComponent::StartInspection; destroys itself
 * when the player exits (Esc / E / right mouse / gamepad B).
 */
UCLASS(NotBlueprintable)
class LOOP9_API AInspectionStageActor : public AActor
{
	GENERATED_BODY()

public:
	AInspectionStageActor();

	/** True while any inspection session is running. */
	static bool IsInspectionActive();

	/** Sets up the view and pauses the game. Returns false if it cannot start. */
	bool BeginInspection(UInspectableComponent* SourceComponent, APlayerController* InController);

	/** Restores everything and destroys this actor. */
	void EndInspection();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Copy of the inspected mesh, rendered as a first-person primitive so it never clips into walls. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> DisplayMesh;

	TWeakObjectPtr<UInspectableComponent> Source;
	TWeakObjectPtr<APlayerController> Controller;
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<USkeletalMeshComponent> HiddenArms;

	/** Camera post-process state before we applied the inspection blur. */
	FPostProcessSettings SavedPostProcess;

	bool bActive = false;
	bool bDidPause = false;
	bool bRestored = false;

	/** Real seconds since the session opened; input is ignored briefly so the
	 *  interact press that opened the view doesn't immediately close it. */
	float TimeActive = 0.0f;

	float RotationSpeed = 0.6f;

	void RestoreState();
	bool WantsExit() const;
};
