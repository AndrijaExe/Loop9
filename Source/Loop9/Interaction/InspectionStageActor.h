#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Scene.h"
#include "UObject/SoftObjectPath.h"
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
UCLASS(NotBlueprintable, Config = Game)
class LOOP9_API AInspectionStageActor : public AActor
{
	GENERATED_BODY()

public:
	AInspectionStageActor();

	/**
	 * Post-process material that blurs the screen except custom-stencil
	 * pixels (the inspected item stays perfectly sharp). Set the path in
	 * DefaultGame.ini; material recipe in EDITOR_TODO.md §4b. When unset
	 * or not found, a depth-of-field fallback is used instead.
	 */
	UPROPERTY(Config)
	FSoftObjectPath BackgroundBlurMaterialPath;

	/** True while any inspection session is running. */
	static bool IsInspectionActive();

	/** Ends the active session if any. Returns true if one was closed. */
	static bool TryEndActiveInspection();

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

	/** Edge-detect exit keys (WasInputKeyJustPressed is unreliable with Enhanced Input while paused). */
	bool bExitKeyWasDown = false;

	void RestoreState();
	bool IsExitKeyDown() const;
};
