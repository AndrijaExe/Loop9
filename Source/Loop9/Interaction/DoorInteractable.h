#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Loop9Interactable.h"
#include "DoorInteractable.generated.h"

class UDoorLockStateAnomalyComponent;

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

	/** Visually ajar / jammed. Prompt is "blocked", not "locked". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (DisplayName = "Blocked From Behind"))
	bool bIsBlocked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	bool bIsOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "1.0"))
	float OpenCloseSpeedDegPerSec = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float OpenYawAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	TObjectPtr<class USoundBase> LockedSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	TObjectPtr<class USoundBase> BlockedSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	TObjectPtr<class USoundBase> OpenSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	TObjectPtr<class USoundBase> CloseSound = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anomaly")
	TObjectPtr<UDoorLockStateAnomalyComponent> DoorLockAnomaly;

	/** Shudder the door in its frame when the handle will not turn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Rattle")
	bool bRattleWhenDenied = true;

	/** Peak yaw of the shudder. A couple of degrees reads as a door held shut. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Rattle", meta = (ClampMin = "0.0", ClampMax = "15.0"))
	float RattleAngleDeg = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Rattle", meta = (ClampMin = "0.05"))
	float RattleDurationSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Rattle", meta = (ClampMin = "1.0"))
	float RattleShakesPerSecond = 13.0f;

	UFUNCTION(BlueprintCallable, Category = "Door")
	bool Interact();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetLocked(bool bLocked);

	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetBlocked(bool bBlocked);

	UFUNCTION(BlueprintCallable, Category = "Door")
	void LockDoor();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void UnlockDoor();

	/** Silent snap to the closed pose. Loop reset must not leave a swung door. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void ResetToClosedBaseline();

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsBlockedFromBehind() const { return bIsBlocked; }

	virtual bool TryInteract_Implementation(APlayerController* InteractingController) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

private:
	bool bIsMoving = false;
	FRotator ClosedRelativeRotation = FRotator::ZeroRotator;
	FRotator OpenRelativeRotation = FRotator::ZeroRotator;
	FRotator TargetRelativeRotation = FRotator::ZeroRotator;

	float RattleTimeRemaining = 0.0f;

	void PlayDoorSound(USoundBase* SoundToPlay);
	USoundBase* ResolveOpenSound() const;
	USoundBase* ResolveCloseSound() const;
	USoundBase* ResolveDeniedSound() const;
	void StartRattle();
	void TickRattle(float DeltaSeconds);
};
