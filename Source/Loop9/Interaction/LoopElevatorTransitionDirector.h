#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Loop/LoopTypes.h"
#include "LoopElevatorTransitionDirector.generated.h"

class ALiftButton;
class ALiftDoorWing;
class ALevelSequenceActor;
class APlayerController;
class ATeleportPoint;
class ULevelSequence;
class ULevelSequencePlayer;

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
 * optional Sequencer playback.
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

	/** Optional camera/light/audio sequence. C++ remains authoritative for doors and teleport. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Sequence")
	TObjectPtr<ULevelSequence> TransitionSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Timing", meta = (ClampMin = "0.1"))
	float DoorCloseTimeoutSeconds = 3.0f;

	/** Time spent travelling after the source doors have fully closed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Timing", meta = (ClampMin = "0.1"))
	float TravelDurationSeconds = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Timing", meta = (ClampMin = "0.0"))
	float FadeDurationSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator Transition|Timing", meta = (ClampMin = "0.1"))
	float DoorOpenTimeoutSeconds = 3.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Elevator Transition")
	void OnTransitionStarted(EButtonType ButtonType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Elevator Transition")
	void OnTravelStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Elevator Transition")
	void OnArrivalStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Elevator Transition")
	void OnTransitionCompleted();

protected:
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
	void PlayOptionalSequence(const ALiftButton* SourceButton);
	void StopOptionalSequence();
	void BindDoorDelegates();
	void UnbindDoorDelegates();
	bool AreSourceDoorsClosed() const;
	bool AreArrivalDoorsOpen() const;
	bool HasValidArrivalDoor() const;
	void ClearTimers();

	ELoopElevatorTransitionPhase Phase = ELoopElevatorTransitionPhase::Idle;
	FLoopElevatorDecision PendingDecision;
	bool bDecisionCommitted = false;
	bool bInputLocked = false;

	TWeakObjectPtr<APlayerController> PlayerController;
	TArray<TWeakObjectPtr<ALiftDoorWing>> SourceDoorWings;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ActiveSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> ActiveSequenceActor;

	FTimerHandle DoorCloseTimeoutHandle;
	FTimerHandle TravelTimerHandle;
	FTimerHandle DoorOpenTimeoutHandle;
};
