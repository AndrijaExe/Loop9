#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Loop9Interactable.h"
#include "Loop9SecretExitDoor.generated.h"

class UStaticMeshComponent;
class USoundBase;
class USoundAttenuation;

/**
 * 1.1 secret ending trigger. Place it as the street door of the ground-floor
 * stub that sits below the missing wall. Interacting asks
 * ULoopManagerSubsystem::TryTriggerSecretExitEnding(); when the manager
 * refuses (wrong floor, wall still there, run over) it behaves like a locked
 * door and plays LockedSound. It never decides on its own whether the exit
 * counts — that stays in the LoopManager so a glitch through geometry cannot
 * award the ending.
 */
UCLASS()
class LOOP9_API ALoop9SecretExitDoor : public AActor, public ILoop9Interactable
{
	GENERATED_BODY()

public:
	ALoop9SecretExitDoor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Secret Exit")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/**
	 * The wall segment whose Hide anomaly exposes the stairwell. Set it: with
	 * it the door opens only while that wall is the missing one. Left empty,
	 * any active Hide anomaly on the floor counts, which a clip through
	 * geometry could exploit.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Secret Exit")
	TObjectPtr<AActor> HiddenWallActor = nullptr;

	/** Plays when the manager accepts the exit, right before the fade / cutscene. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret Exit|Audio")
	TObjectPtr<USoundBase> OpenSound = nullptr;

	/** Plays when the door refuses (wall still there, floor too low). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret Exit|Audio")
	TObjectPtr<USoundBase> LockedSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret Exit|Audio")
	TObjectPtr<USoundAttenuation> Attenuation = nullptr;

	/** Blueprint hook for a door swing / light spill before the ending takes over. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Secret Exit")
	void OnExitAccepted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Secret Exit")
	void OnExitRefused();

	virtual bool TryInteract_Implementation(APlayerController* InteractingController) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

private:
	bool bConsumed = false;
};
