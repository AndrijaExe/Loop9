#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InspectionStageActor.generated.h"

class APlayerController;
class UCameraComponent;
class UInspectableComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Internal actor that runs a single item-inspection session in an isolated
 * "black room": the stage teleports far above the map, a black box encloses
 * a copy of the item's mesh, a key + fill light illuminate it and the
 * player's view switches to the stage camera. The game is paused and the
 * player rotates the item with the mouse / right gamepad stick.
 *
 * No post-process tricks (blur/DOF/custom depth) — the item renders normally,
 * fully opaque and sharp, against a guaranteed black background.
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

	/** Exposure bias (stops) for the black room's manual exposure. Higher = brighter. Tunable via DefaultGame.ini. */
	UPROPERTY(Config)
	float ExposureBias = 0.0f;

	/** Key light intensity (candela). Sized for manual exposure with default physical-camera settings. Tunable via DefaultGame.ini. */
	UPROPERTY(Config)
	float KeyLightIntensityCandela = 300.0f;

	/** Fill light intensity (candela). Tunable via DefaultGame.ini. */
	UPROPERTY(Config)
	float FillLightIntensityCandela = 75.0f;

	/** True while any inspection session is running. */
	static bool IsInspectionActive();

	/** Ends the active session if any. Returns true if one was closed. */
	static bool TryEndActiveInspection();

	/** Sets up the black room, switches the view and pauses the game. Returns false if it cannot start. */
	bool BeginInspection(UInspectableComponent* SourceComponent, APlayerController* InController);

	/** Restores everything and destroys this actor. */
	void EndInspection();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Rotated by input; the display mesh hangs off it so the item spins around its visual center. */
	UPROPERTY()
	TObjectPtr<USceneComponent> ItemPivot;

	/** Copy of the inspected mesh. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> DisplayMesh;

	/** Inverted (negative-scale) black cube enclosing the stage — the background. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Backdrop;

	/** The view during inspection; looks down +X at the item. */
	UPROPERTY()
	TObjectPtr<UCameraComponent> StageCamera;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> KeyLight;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> FillLight;

	TWeakObjectPtr<UInspectableComponent> Source;
	TWeakObjectPtr<APlayerController> Controller;

	/** View target before the inspection started, restored on exit. */
	TWeakObjectPtr<AActor> PreviousViewTarget;

	bool bActive = false;
	bool bDidPause = false;
	bool bDidIgnoreInput = false;
	bool bPreviousFullTickWhenPaused = false;
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
