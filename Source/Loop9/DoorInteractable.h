#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Loop9Interactable.h"
#include "DoorInteractable.generated.h"

UCLASS()
class LOOP9_API ADoorInteractable : public AActor, public ILoop9Interactable
{
	GENERATED_BODY()

public:
	ADoorInteractable();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<class USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<class UStaticMeshComponent> DoorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bIsLocked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	bool bIsOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "1.0"))
	float OpenCloseSpeedDegPerSec = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float OpenYawAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	TObjectPtr<class USoundBase> LockedSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	TObjectPtr<class USoundBase> OpenCloseSound = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Door")
	bool Interact();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetLocked(bool bLocked);

	UFUNCTION(BlueprintCallable, Category = "Door")
	void LockDoor();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void UnlockDoor();

	virtual bool TryInteract_Implementation(APlayerController* InteractingController) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

private:
	bool bIsMoving = false;
	FRotator ClosedRelativeRotation = FRotator::ZeroRotator;
	FRotator OpenRelativeRotation = FRotator::ZeroRotator;
	FRotator TargetRelativeRotation = FRotator::ZeroRotator;

	void PlayDoorSound(USoundBase* SoundToPlay);
};
