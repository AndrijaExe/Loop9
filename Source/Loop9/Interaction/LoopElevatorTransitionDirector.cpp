#include "Interaction/LoopElevatorTransitionDirector.h"

#include "Loop9.h"
#include "Controllers/Loop9BasePlayerController.h"
#include "Controllers/Loop9PlayerController.h"
#include "Interaction/LiftButton.h"
#include "LiftDoorWing.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "TeleportPoint.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

ALoopElevatorTransitionDirector::ALoopElevatorTransitionDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));
}

void ALoopElevatorTransitionDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateLookBlend(DeltaTime);
}

bool ALoopElevatorTransitionDirector::BeginTransition(
	ALiftButton* SourceButton,
	APlayerController* InteractingController)
{
	if (IsTransitionInProgress() || !SourceButton || !InteractingController)
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULoopManagerSubsystem* LoopManager =
		GameInstance ? GameInstance->GetSubsystem<ULoopManagerSubsystem>() : nullptr;
	if (!LoopManager)
	{
		return false;
	}

	const EButtonType ButtonType = SourceButton->ButtonType == ELiftButtonType::Increment
		? EButtonType::Increment
		: EButtonType::Reset;
	PendingDecision = LoopManager->ResolveElevatorDecision(ButtonType);
	if (!PendingDecision.IsValid())
	{
		return false;
	}

	PlayerController = InteractingController;
	bDecisionCommitted = false;
	Phase = ELoopElevatorTransitionPhase::ClosingDoors;

	SourceDoorWings.Reset();
	for (ALiftDoorWing* DoorWing : SourceButton->TransitionDoorWings)
	{
		if (IsValid(DoorWing))
		{
			SourceDoorWings.AddUnique(DoorWing);
		}
	}

	BindDoorDelegates();
	LockPlayerInput(true);
	PlayButtonPressSound(SourceButton);
	BeginLookBlend(ResolveClosingLookTarget(InteractingController));
	OnTransitionStarted(ButtonType);
	RemoveLegacyBlinkOverlay();
	if (IsActorBeingDestroyed() || Phase != ELoopElevatorTransitionPhase::ClosingDoors)
	{
		return true;
	}
	for (const TWeakObjectPtr<ALiftDoorWing>& DoorWing : SourceDoorWings)
	{
		if (DoorWing.IsValid())
		{
			DoorWing->CloseDoorWing();
		}
	}
	for (ALiftDoorWing* DoorWing : ArrivalDoorWings)
	{
		if (IsValid(DoorWing))
		{
			DoorWing->CloseDoorWing();
		}
	}

	if (AreSourceDoorsClosed())
	{
		BeginTravel();
	}
	else if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DoorCloseTimeoutHandle,
			this,
			&ALoopElevatorTransitionDirector::BeginTravel,
			DoorCloseTimeoutSeconds,
			false);
	}

	return true;
}

void ALoopElevatorTransitionDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!IsTransitionInProgress())
	{
		Super::EndPlay(EndPlayReason);
		return;
	}

	if (IsTransitionInProgress() && EndPlayReason == EEndPlayReason::Destroyed)
	{
		AbortTransition(true);
		Super::EndPlay(EndPlayReason);
		return;
	}

	ClearTimers();
	UnbindDoorDelegates();
	StopTravelSound();
	StopCameraFade();
	FinishLookBlend();
	LockPlayerInput(false);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GameInstance->GetSubsystem<ULoopManagerSubsystem>())
		{
			if (bDecisionCommitted)
			{
				if (LoopManager->FinishElevatorTransition(PendingDecision.DecisionId))
				{
					if (EndPlayReason == EEndPlayReason::RemovedFromWorld)
					{
						LoopManager->CompleteDeferredEnding();
					}
					else
					{
						LoopManager->CancelDeferredEnding();
					}
				}
			}
			else
			{
				LoopManager->CancelElevatorDecision(PendingDecision.DecisionId);
			}
		}
	}
	Phase = ELoopElevatorTransitionPhase::Idle;

	Super::EndPlay(EndPlayReason);
}

void ALoopElevatorTransitionDirector::HandleDoorMovementFinished(bool bIsOpen)
{
	if (Phase == ELoopElevatorTransitionPhase::ClosingDoors && !bIsOpen && AreSourceDoorsClosed())
	{
		BeginTravel();
	}
	else if (Phase == ELoopElevatorTransitionPhase::OpeningDoors && bIsOpen && AreArrivalDoorsOpen())
	{
		CompleteTransition();
	}
}

void ALoopElevatorTransitionDirector::BeginTravel()
{
	if (Phase != ELoopElevatorTransitionPhase::ClosingDoors)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DoorCloseTimeoutHandle);
	}

	FinishLookBlend();
	Phase = ELoopElevatorTransitionPhase::Travelling;
	OnTravelStarted();
	RemoveLegacyBlinkOverlay();
	if (IsActorBeingDestroyed() || Phase != ELoopElevatorTransitionPhase::Travelling)
	{
		return;
	}
	StartTravelSound();
	// One continuous blackout: fade out, stay black until CommitAndArrive, then fade in once.
	StartCameraFade(0.0f, 1.0f, FadeOutDurationSeconds);

	// The cabin we leave should look usable again on the next visit (especially the dark lift),
	// but wait a beat so reopen does not feel simultaneous with the close.
	if (UWorld* World = GetWorld())
	{
		const float ReopenDelay = FMath::Max(0.0f, AbandonedDoorReopenDelaySeconds);
		if (ReopenDelay <= KINDA_SMALL_NUMBER)
		{
			ReopenAbandonedSourceDoors();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				AbandonedDoorReopenHandle,
				this,
				&ALoopElevatorTransitionDirector::ReopenAbandonedSourceDoors,
				ReopenDelay,
				false);
		}

		// Never cut the blackout shorter than the fade-to-black itself.
		const float TravelDelay = FMath::Max(TravelDurationSeconds, FadeOutDurationSeconds);
		World->GetTimerManager().SetTimer(
			TravelTimerHandle,
			this,
			&ALoopElevatorTransitionDirector::CommitAndArrive,
			TravelDelay,
			false);
	}
	else
	{
		AbortTransition(true);
	}
}

void ALoopElevatorTransitionDirector::CommitAndArrive()
{
	if (Phase != ELoopElevatorTransitionPhase::Travelling)
	{
		return;
	}

	StopTravelSound();

	// Teleport can reset the camera manager fade; hold pure black first so the
	// player never sees a mid-transition flash / triple blink.
	HoldCameraBlack();

	UGameInstance* GameInstance = GetGameInstance();
	ULoopManagerSubsystem* LoopManager =
		GameInstance ? GameInstance->GetSubsystem<ULoopManagerSubsystem>() : nullptr;
	if (!LoopManager
		|| !LoopManager->CommitElevatorDecision(PendingDecision, false, true))
	{
		AbortTransition(true);
		return;
	}
	bDecisionCommitted = true;

	APlayerController* PC = PlayerController.Get();
	ACharacter* PlayerCharacter = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	if (PC && PlayerCharacter && IsValid(LitElevatorArrivalPoint) && HasValidArrivalDoor())
	{
		const FVector ArrivalLocation = LitElevatorArrivalPoint->GetTeleportLocation();
		const FRotator ArrivalRotation = LitElevatorArrivalPoint->GetTeleportRotation();
		PlayerCharacter->SetActorLocation(ArrivalLocation, false, nullptr, ETeleportType::TeleportPhysics);
		PlayerCharacter->SetActorRotation(ArrivalRotation, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(ArrivalRotation);
		LookBlendStartRotation = ArrivalRotation;
		LookBlendTargetRotation = ArrivalRotation;
		HoldCameraBlack();
	}
	else
	{
		UE_LOG(LogLoop9, Warning, TEXT(
			"Elevator transition '%s': safe arrival references/player missing; using legacy Exit teleport."),
			*GetName());
		LoopManager->TeleportPlayerToExit();
		HoldCameraBlack();
	}

	Phase = ELoopElevatorTransitionPhase::OpeningDoors;
	OnArrivalStarted();
	RemoveLegacyBlinkOverlay();
	if (IsActorBeingDestroyed() || Phase != ELoopElevatorTransitionPhase::OpeningDoors)
	{
		return;
	}
	StartCameraFade(1.0f, 0.0f, FadeInDurationSeconds);

	for (ALiftDoorWing* DoorWing : ArrivalDoorWings)
	{
		if (IsValid(DoorWing))
		{
			DoorWing->OpenDoorWing();
		}
	}

	if (AreArrivalDoorsOpen())
	{
		CompleteTransition();
	}
	else if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DoorOpenTimeoutHandle,
			this,
			&ALoopElevatorTransitionDirector::HandleDoorOpenTimeout,
			DoorOpenTimeoutSeconds,
			false);
	}
}

void ALoopElevatorTransitionDirector::HandleDoorOpenTimeout()
{
	if (Phase != ELoopElevatorTransitionPhase::OpeningDoors)
	{
		return;
	}

	UE_LOG(LogLoop9, Warning, TEXT(
		"Elevator transition '%s': arrival doors timed out; forcing safe open state."),
		*GetName());
	for (ALiftDoorWing* DoorWing : ArrivalDoorWings)
	{
		if (IsValid(DoorWing))
		{
			DoorWing->ForceOpenState();
		}
	}
	CompleteTransition();
}

void ALoopElevatorTransitionDirector::CompleteTransition()
{
	if (Phase == ELoopElevatorTransitionPhase::Idle)
	{
		return;
	}

	ClearTimers();
	UnbindDoorDelegates();
	StopTravelSound();
	FinishLookBlend();
	LockPlayerInput(false);
	Phase = ELoopElevatorTransitionPhase::Idle;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GameInstance->GetSubsystem<ULoopManagerSubsystem>())
		{
			if (LoopManager->FinishElevatorTransition(PendingDecision.DecisionId))
			{
				LoopManager->CompleteDeferredEnding();
				OnTransitionCompleted();
				return;
			}
		}
	}

	OnTransitionCompleted();
}

void ALoopElevatorTransitionDirector::AbortTransition(bool bCommitWithInstantFallback)
{
	UE_LOG(LogLoop9, Warning, TEXT("Elevator transition '%s' aborted; applying safe fallback."), *GetName());

	bool bShouldCompleteDeferredEnding = false;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GameInstance->GetSubsystem<ULoopManagerSubsystem>())
		{
			if (bCommitWithInstantFallback && !bDecisionCommitted && PendingDecision.IsValid())
			{
				bDecisionCommitted = LoopManager->CommitElevatorDecision(PendingDecision, true, false);
			}
			bShouldCompleteDeferredEnding =
				bDecisionCommitted
				&& LoopManager->FinishElevatorTransition(PendingDecision.DecisionId);
		}
	}

	ClearTimers();
	UnbindDoorDelegates();
	StopTravelSound();
	StopCameraFade();
	FinishLookBlend();
	LockPlayerInput(false);
	Phase = ELoopElevatorTransitionPhase::Idle;

	if (bShouldCompleteDeferredEnding)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (ULoopManagerSubsystem* LoopManager = GameInstance->GetSubsystem<ULoopManagerSubsystem>())
			{
				LoopManager->CompleteDeferredEnding();
			}
		}
	}
}

void ALoopElevatorTransitionDirector::LockPlayerInput(bool bLock)
{
	APlayerController* PC = PlayerController.Get();
	if (!PC || bInputLocked == bLock)
	{
		return;
	}

	PC->SetIgnoreMoveInput(bLock);
	PC->SetIgnoreLookInput(bLock);
	bInputLocked = bLock;

	if (bLock)
	{
		if (ALoop9BasePlayerController* LoopPC = Cast<ALoop9BasePlayerController>(PC))
		{
			LoopPC->ClearInteractionPrompt();
		}
	}
}

void ALoopElevatorTransitionDirector::RemoveLegacyBlinkOverlay()
{
	if (ALoop9PlayerController* LoopPC = Cast<ALoop9PlayerController>(PlayerController.Get()))
	{
		LoopPC->RemoveBlinkOverlay();
	}
}

void ALoopElevatorTransitionDirector::StartCameraFade(float FromAlpha, float ToAlpha, float DurationSeconds)
{
	if (APlayerController* PC = PlayerController.Get())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(
				FromAlpha,
				ToAlpha,
				FMath::Max(0.0f, DurationSeconds),
				FLinearColor::Black,
				false,
				ToAlpha >= 1.0f - KINDA_SMALL_NUMBER);
		}
	}
}

void ALoopElevatorTransitionDirector::HoldCameraBlack()
{
	StartCameraFade(1.0f, 1.0f, 0.0f);
}

void ALoopElevatorTransitionDirector::StopCameraFade()
{
	if (APlayerController* PC = PlayerController.Get())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StopCameraFade();
		}
	}
}

void ALoopElevatorTransitionDirector::BindDoorDelegates()
{
	for (const TWeakObjectPtr<ALiftDoorWing>& DoorWing : SourceDoorWings)
	{
		if (DoorWing.IsValid())
		{
			DoorWing->OnMovementFinished.RemoveDynamic(
				this,
				&ALoopElevatorTransitionDirector::HandleDoorMovementFinished);
			DoorWing->OnMovementFinished.AddDynamic(
				this,
				&ALoopElevatorTransitionDirector::HandleDoorMovementFinished);
		}
	}
	for (ALiftDoorWing* DoorWing : ArrivalDoorWings)
	{
		if (IsValid(DoorWing))
		{
			DoorWing->OnMovementFinished.RemoveDynamic(
				this,
				&ALoopElevatorTransitionDirector::HandleDoorMovementFinished);
			DoorWing->OnMovementFinished.AddDynamic(
				this,
				&ALoopElevatorTransitionDirector::HandleDoorMovementFinished);
		}
	}
}

void ALoopElevatorTransitionDirector::UnbindDoorDelegates()
{
	for (const TWeakObjectPtr<ALiftDoorWing>& DoorWing : SourceDoorWings)
	{
		if (DoorWing.IsValid())
		{
			DoorWing->OnMovementFinished.RemoveDynamic(
				this,
				&ALoopElevatorTransitionDirector::HandleDoorMovementFinished);
		}
	}
	for (ALiftDoorWing* DoorWing : ArrivalDoorWings)
	{
		if (IsValid(DoorWing))
		{
			DoorWing->OnMovementFinished.RemoveDynamic(
				this,
				&ALoopElevatorTransitionDirector::HandleDoorMovementFinished);
		}
	}
	SourceDoorWings.Reset();
}

bool ALoopElevatorTransitionDirector::AreSourceDoorsClosed() const
{
	for (const TWeakObjectPtr<ALiftDoorWing>& DoorWing : SourceDoorWings)
	{
		if (DoorWing.IsValid() && (DoorWing->ShouldMove || DoorWing->IsOpened))
		{
			return false;
		}
	}
	return true;
}

bool ALoopElevatorTransitionDirector::AreArrivalDoorsOpen() const
{
	for (ALiftDoorWing* DoorWing : ArrivalDoorWings)
	{
		if (!IsValid(DoorWing))
		{
			continue;
		}

		if (DoorWing->ShouldMove || !DoorWing->IsOpened)
		{
			return false;
		}
	}
	return true;
}

bool ALoopElevatorTransitionDirector::HasValidArrivalDoor() const
{
	for (ALiftDoorWing* DoorWing : ArrivalDoorWings)
	{
		if (IsValid(DoorWing))
		{
			return true;
		}
	}
	return false;
}

bool ALoopElevatorTransitionDirector::IsArrivalDoor(const ALiftDoorWing* DoorWing) const
{
	if (!IsValid(DoorWing))
	{
		return false;
	}

	for (ALiftDoorWing* ArrivalDoor : ArrivalDoorWings)
	{
		if (ArrivalDoor == DoorWing)
		{
			return true;
		}
	}
	return false;
}

void ALoopElevatorTransitionDirector::ReopenAbandonedSourceDoors()
{
	for (const TWeakObjectPtr<ALiftDoorWing>& DoorWing : SourceDoorWings)
	{
		if (DoorWing.IsValid() && !IsArrivalDoor(DoorWing.Get()))
		{
			DoorWing->OpenDoorWing();
		}
	}
}

void ALoopElevatorTransitionDirector::BeginLookBlend(const FRotator& TargetRotation)
{
	APlayerController* PC = PlayerController.Get();
	if (!PC)
	{
		bLookBlendActive = false;
		SetActorTickEnabled(false);
		return;
	}

	LookBlendStartRotation = PC->GetControlRotation();
	LookBlendTargetRotation = TargetRotation;
	LookBlendElapsedSeconds = 0.0f;
	bLookBlendActive = true;
	SetActorTickEnabled(true);
}

void ALoopElevatorTransitionDirector::FinishLookBlend()
{
	if (APlayerController* PC = PlayerController.Get())
	{
		PC->SetControlRotation(LookBlendTargetRotation);
	}

	bLookBlendActive = false;
	LookBlendElapsedSeconds = LookBlendDurationSeconds;
	if (Phase == ELoopElevatorTransitionPhase::Idle)
	{
		SetActorTickEnabled(false);
	}
}

void ALoopElevatorTransitionDirector::UpdateLookBlend(float DeltaTime)
{
	if (!bLookBlendActive)
	{
		return;
	}

	APlayerController* PC = PlayerController.Get();
	if (!PC)
	{
		bLookBlendActive = false;
		return;
	}

	LookBlendElapsedSeconds += DeltaTime;
	const float Duration = FMath::Max(LookBlendDurationSeconds, KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(LookBlendElapsedSeconds / Duration, 0.0f, 1.0f);
	const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
	PC->SetControlRotation(
		FMath::Lerp(LookBlendStartRotation, LookBlendTargetRotation, SmoothAlpha));

	if (Alpha >= 1.0f)
	{
		bLookBlendActive = false;
		PC->SetControlRotation(LookBlendTargetRotation);
	}
}

FRotator ALoopElevatorTransitionDirector::ResolveClosingLookTarget(
	APlayerController* InteractingController) const
{
	if (!InteractingController)
	{
		return FRotator::ZeroRotator;
	}

	// Lit path: source doors are the arrival doors — ease straight to the authored arrival facing.
	bool bSourceIsArrivalCabin = SourceDoorWings.Num() > 0;
	for (const TWeakObjectPtr<ALiftDoorWing>& DoorWing : SourceDoorWings)
	{
		if (!DoorWing.IsValid() || !IsArrivalDoor(DoorWing.Get()))
		{
			bSourceIsArrivalCabin = false;
			break;
		}
	}
	if (bSourceIsArrivalCabin && IsValid(LitElevatorArrivalPoint))
	{
		return LitElevatorArrivalPoint->GetTeleportRotation();
	}

	FVector DoorCenter = FVector::ZeroVector;
	int32 DoorCount = 0;
	for (const TWeakObjectPtr<ALiftDoorWing>& DoorWing : SourceDoorWings)
	{
		if (DoorWing.IsValid())
		{
			DoorCenter += DoorWing->GetActorLocation();
			++DoorCount;
		}
	}

	const APawn* Pawn = InteractingController->GetPawn();
	const FVector EyeLocation = Pawn
		? Pawn->GetPawnViewLocation()
		: InteractingController->GetFocalLocation();
	if (DoorCount > 0)
	{
		DoorCenter /= static_cast<float>(DoorCount);
		return (DoorCenter - EyeLocation).Rotation();
	}

	return InteractingController->GetControlRotation();
}

void ALoopElevatorTransitionDirector::PlayButtonPressSound(const ALiftButton* SourceButton)
{
	if (!IsValid(ButtonPressSound) || !SourceButton)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		this,
		ButtonPressSound,
		SourceButton->GetActorLocation(),
		FRotator::ZeroRotator,
		FMath::Max(0.0f, ButtonPressSoundVolume));
}

void ALoopElevatorTransitionDirector::StartTravelSound()
{
	StopTravelSound();
	if (!IsValid(TravelSound))
	{
		return;
	}

	ActiveTravelAudio = UGameplayStatics::SpawnSound2D(
		this,
		TravelSound,
		FMath::Max(0.0f, TravelSoundVolume),
		1.0f,
		0.0f,
		nullptr,
		false,
		true);
}

void ALoopElevatorTransitionDirector::StopTravelSound()
{
	if (!IsValid(ActiveTravelAudio))
	{
		ActiveTravelAudio = nullptr;
		return;
	}

	if (TravelSoundFadeOutSeconds > KINDA_SMALL_NUMBER)
	{
		ActiveTravelAudio->FadeOut(TravelSoundFadeOutSeconds, 0.0f);
	}
	else
	{
		ActiveTravelAudio->Stop();
	}
	ActiveTravelAudio = nullptr;
}

void ALoopElevatorTransitionDirector::ClearTimers()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(DoorCloseTimeoutHandle);
		TimerManager.ClearTimer(TravelTimerHandle);
		TimerManager.ClearTimer(DoorOpenTimeoutHandle);
		TimerManager.ClearTimer(AbandonedDoorReopenHandle);
	}
}
