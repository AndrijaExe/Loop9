#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MenuSceneLibrary.generated.h"

/**
 * Helpers for the main menu background scene, which is driven by the MainMenu
 * level Blueprint rather than a C++ director. These exist so the Blueprint can
 * ask where the figure should stand instead of storing a location that stops
 * being right the moment a light or a piece of set dressing moves.
 */
UCLASS()
class LOOP9_API UMenuSceneLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Where a standing figure belongs directly beneath a light.
	 *
	 * Traces down from the light to the floor, lifts the result by FloorClearance
	 * so a Character spawns on the floor rather than through it, and yaws the
	 * figure toward FaceTarget. Feed the result straight into SpawnActor in place
	 * of a stored StartLoc / StartRot.
	 *
	 * @param LightActor       Light to stand under. A light component on it is
	 *                         preferred over the actor pivot, since a ceiling
	 *                         fixture's pivot is often nowhere near its bulb.
	 * @param FaceTarget       Actor to turn toward. Leave empty to use the actor
	 *                         tagged MenuCamera, which is what the menu wants.
	 * @param FloorClearance   Capsule half height of the spawned pawn.
	 * @param ForwardOffset    Positive nudges the figure toward FaceTarget, for
	 *                         when dead centre reads as too composed.
	 * @param MaxDropDistance  How far down to look for a floor before giving up.
	 */
	UFUNCTION(BlueprintPure, Category = "Menu Scene",
		meta = (WorldContext = "WorldContextObject",
			DisplayName = "Get Figure Transform Under Light",
			AdvancedDisplay = "ForwardOffset,MaxDropDistance"))
	static FTransform GetFigureTransformUnderLight(
		UObject* WorldContextObject,
		AActor* LightActor,
		AActor* FaceTarget = nullptr,
		float FloorClearance = 96.0f,
		float ForwardOffset = 0.0f,
		float MaxDropDistance = 1200.0f);

	/** World location of a light actor's brightest component, or its pivot. */
	UFUNCTION(BlueprintPure, Category = "Menu Scene")
	static FVector GetLightWorldLocation(AActor* LightActor);
};
