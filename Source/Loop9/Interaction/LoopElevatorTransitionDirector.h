#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Loop/LoopTypes.h"
#include "LoopElevatorTransitionDirector.generated.h"

class ALiftButton;
class ALiftDoorWing;
class APlayerController;
class ATeleportPoint;
class UAudioComponent;
class USoundBase;

UENUM(BlueprintType)
enum class ELoopElevatorTransitionPhase : uint8
{
	Idle,
	ClosingDoors,
	Travelling,
	OpeningDoors
};

/**
 * World presentation layer for elevator transitions. Gameplay state remains in
 * ULoopManagerSubsystem; this actor owns doors, fades, teleport timing and
 * transition audio.
 */
UCLASS(Blueprintable)
class LOOP9_API ALoopElevatorTransitionDirector : public AActor
{
	GENERATED_BODY()

public:
	ALoopElevatorTransitionDirector();

	bool BeginTransition(ALiftButton* SourceButton, APlayerController* InteractingController);

	UFUNCTION(BlueprintPure, Category = "Elevator Transition")
	bool IsTransitionInProgress() const { return Phase != ELoopElevatorTransitionPhase::Idle; }

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Elevator Transition|Arrival")
	TObjectPtr<ATeleportPoint> LitElevatorArrivalPoint;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Elevator Transition|Arrival")
	TArray<TObjectPtr<ALiftDoorWing>> ArrivalDoorWings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Timing", meta = (ClampMin = "0.1"))
	float DoorCloseTimeoutSeconds = 3.0f;

	/** Time spent travelling after the source doors have fully closed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Timing", meta = (ClampMin = "0.1"))
	float TravelDurationSeconds = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Timing", meta = (ClampMin = "0.0"))
	float FadeDurationSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Timing", meta = (ClampMin = "0.1"))
	float DoorOpenTimeoutSeconds = 3.0f;

	/** How long the view eases from the player's look direction toward the blend target while doors close. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Camera", meta = (ClampMin = "0.05"))
	float LookBlendDurationSeconds = 1.25f;

	/** Delay before reopening the cabin you left (e.g. dark lift) so it does not pop open instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Timing", meta = (ClampMin = "0.0"))
	float AbandonedDoorReopenDelaySeconds = 1.25f;

	/** One-shot played at the selected lift button as soon as the transition starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Audio")
	TObjectPtr<USoundBase> ButtonPressSound;

	/** Starts after the source doors close and stops/fades when the lift arrives. Looping assets work best. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Audio")
	TObjectPtr<USoundBase> TravelSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Audio", meta = (ClampMin = "0.0"))
	float ButtonPressSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Audio", meta = (ClampMin = "0.0"))
	float TravelSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Audio", meta = (ClampMin = "0.0"))
	float TravelSoundFadeOutSeconds = 0.15f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Elevator Transition")
	void OnTransitionStarted(EButtonType ButtonType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Elevator Transition")
	void OnTravelStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Elevator Transition")
	void OnArrivalStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Elevator Transition")
	void OnTransitionCompleted();

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleDoorMovementFinished(bool bIsOpen);

	void BeginTravel();
	void CommitAndArrive();
	void CompleteTransition();
	void HandleDoorOpenTimeout();
	void AbortTransition(bool bCommitWithInstantFallback);
	void LockPlayerInput(bool bLock);
	void StartCameraFade(float FromAlpha, float ToAlpha);
	void StopCameraFade();
	void BindDoorDelegates();
	void UnbindDoorDelegates();
	bool AreSourceDoorsClosed() const;
	bool AreArrivalDoorsOpen() const;
	bool HasValidArrivalDoor() const;
	bool IsArrivalDoor(const ALiftDoorWing* DoorWing) const;
	void ReopenAbandonedSourceDoors();
	void BeginLookBlend(const FRotator& TargetRotation);
	void FinishLookBlend();
	void UpdateLookBlend(float DeltaTime);
	FRotator ResolveClosingLookTarget(APlayerController* InteractingController) const;
	void PlayButtonPressSound(const ALiftButton* SourceButton);
	void StartTravelSound();
	void StopTravelSound();
	void ClearTimers();

	ELoopElevatorTransitionPhase Phase = ELoopElevatorTransitionPhase::Idle;
	FLoopElevatorDecision PendingDecision;
	bool bDecisionCommitted = false;
	bool bInputLocked = false;
	bool bLookBlendActive = false;
	float LookBlendElapsedSeconds = 0.0f;
	FRotator LookBlendStartRotation = FRotator::ZeroRotator;
	FRotator LookBlendTargetRotation = FRotator::ZeroRotator;

	TWeakObjectPtr<APlayerController> PlayerController;
	TArray<TWeakObjectPtr<ALiftDoorWing>> SourceDoorWings;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveTravelAudio;

	FTimerHandle DoorCloseTimeoutHandle;
	FTimerHandle TravelTimerHandle;
	FTimerHandle DoorOpenTimeoutHandle;
	FTimerHandle AbandonedDoorReopenHandle;
};
