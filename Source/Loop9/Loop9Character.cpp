// Copyright Epic Games, Inc. All Rights Reserved.

#include "Loop9Character.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Loop9.h"
#include "Interaction/Loop9Interactable.h"
#include "Interaction/InspectableComponent.h"
#include "Interaction/InspectionStageActor.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Sound/SoundBase.h"

ALoop9Character::ALoop9Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
}

bool ALoop9Character::IsGameplayPresentationLocked() const
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	ULoopManagerSubsystem* LoopManager =
		GameInstance ? GameInstance->GetSubsystem<ULoopManagerSubsystem>() : nullptr;
	return LoopManager
		&& (LoopManager->IsElevatorTransitionPending() || LoopManager->bGameFinished);
}

void ALoop9Character::BeginPlay()
{
	Super::BeginPlay();

	// Ensure proper input mode for gameplay
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		// Ensure input wasn't left locked by previous sequences/ending
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);

		// Set input mode to Game Only (hide cursor, enable game input)
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		PC->bEnableClickEvents = false;
		PC->bEnableMouseOverEvents = false;

		UE_LOG(LogLoop9, Log, TEXT("Loop9Character: Input mode set to Game Only, cursor hidden"));

		// Smooth fade-in after level has fully loaded (works with MoviePlayer loading screen)
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.6f, FLinearColor::Black, false, true);
		}
	}
}

void ALoop9Character::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TryPlayFootstep(DeltaSeconds);
}

void ALoop9Character::TryPlayFootstep(float DeltaSeconds)
{
	if (FootstepSounds.Num() == 0)
	{
		return;
	}

	if (!GetCharacterMovement() || !GetCharacterMovement()->IsMovingOnGround())
	{
		FootstepTimer = 0.0f;
		return;
	}

	const FVector HorizontalVelocity = FVector(GetVelocity().X, GetVelocity().Y, 0.0f);
	const float Speed = HorizontalVelocity.Size();
	if (Speed < MinFootstepSpeed)
	{
		FootstepTimer = 0.0f;
		return;
	}

	const bool bIsRunning = Speed > 350.0f;
	const float TargetInterval = bIsRunning ? FootstepIntervalRun : FootstepIntervalWalk;

	FootstepTimer += DeltaSeconds;
	if (FootstepTimer < TargetInterval)
	{
		return;
	}

	FootstepTimer = 0.0f;

	const int32 Index = FMath::RandRange(0, FootstepSounds.Num() - 1);
	USoundBase* ChosenSound = FootstepSounds[Index];
	if (!ChosenSound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(FootstepPitchMin, FootstepPitchMax);
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ChosenSound, GetActorLocation(), GetActorRotation(), FootstepVolume, Pitch);
}

void ALoop9Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ALoop9Character::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ALoop9Character::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALoop9Character::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALoop9Character::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ALoop9Character::LookInput);

		// Interact
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ALoop9Character::OnInteract);
		}

		// Pause
		if (PauseAction)
		{
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &ALoop9Character::OnPause);
		}
	}
	else
	{
		UE_LOG(LogLoop9, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	RegisterGamepadFallbackContext();
}

void ALoop9Character::RegisterGamepadFallbackContext()
{
	// IMC_Default (from the FP template) already maps move/look/jump to the
	// gamepad, but the custom actions (Interact, Pause, Sprint) are
	// keyboard-only assets. Build a transient context here so the game is
	// playable with a controller / on Steam Deck without editing content.
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}

	if (!GamepadFallbackContext)
	{
		GamepadFallbackContext = NewObject<UInputMappingContext>(this, TEXT("IMC_GamepadFallback"));
		AddGamepadFallbackMappings(GamepadFallbackContext);
	}

	if (!Subsystem->HasMappingContext(GamepadFallbackContext))
	{
		Subsystem->AddMappingContext(GamepadFallbackContext, 0);
	}
}

void ALoop9Character::AddGamepadFallbackMappings(UInputMappingContext* Context)
{
	if (InteractAction)
	{
		// X on Xbox / Square on PlayStation - the usual "use" button.
		Context->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
	}

	if (PauseAction)
	{
		// Start / Menu button.
		Context->MapKey(PauseAction, EKeys::Gamepad_Special_Right);
	}
}


void ALoop9Character::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ALoop9Character::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	float Sensitivity = 1.0f;
	float InvertY = 1.0f;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (const ULoop9GameSettingsSubsystem* GameSettings = GI->GetSubsystem<ULoop9GameSettingsSubsystem>())
		{
			Sensitivity = GameSettings->GetMouseSensitivity();
			InvertY = GameSettings->IsYAxisInverted() ? -1.0f : 1.0f;
		}
	}

	// pass the axis values to the aim input
	// Invert Y so mouse-up looks up (standard FPS behavior)
	DoAim(LookAxisVector.X * Sensitivity, -LookAxisVector.Y * Sensitivity * InvertY);

}

void ALoop9Character::DoAim(float Yaw, float Pitch)
{
	if (IsGameplayPresentationLocked())
	{
		return;
	}

	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ALoop9Character::DoMove(float Right, float Forward)
{
	if (IsGameplayPresentationLocked())
	{
		return;
	}

	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ALoop9Character::DoJumpStart()
{
	if (IsGameplayPresentationLocked())
	{
		return;
	}

	// pass Jump to the character
	Jump();
}

void ALoop9Character::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void ALoop9Character::OnInteract()
{
	if (AInspectionStageActor::TryEndActiveInspection())
	{
		return;
	}

	PerformInteractTrace();
}

void ALoop9Character::OnPause()
{
	UE_LOG(LogLoop9, Log, TEXT("Pause key pressed"));

	if (IsGameplayPresentationLocked())
	{
		return;
	}

	// Esc during item inspection closes the inspect view instead of opening pause.
	if (AInspectionStageActor::TryEndActiveInspection())
	{
		return;
	}

	if (!PauseMenuWidgetClass)
	{
		UE_LOG(LogLoop9, Warning, TEXT("PauseMenuWidgetClass not set!"));
		return;
	}

	// Check if already paused
	bool bIsGamePaused = UGameplayStatics::IsGamePaused(GetWorld());

	if (!bIsGamePaused)
	{
		// Create and show pause menu
		PauseMenuInstance = CreateWidget<UUserWidget>(GetWorld(), PauseMenuWidgetClass);

		if (PauseMenuInstance)
		{
			PauseMenuInstance->AddToViewport(10); // High Z-order

			// Pause the game
			UGameplayStatics::SetGamePaused(GetWorld(), true);

			// Set input mode to UI
			APlayerController* PC = Cast<APlayerController>(GetController());
			if (PC)
			{
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(PauseMenuInstance->TakeWidget());
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
			}

			UE_LOG(LogLoop9, Log, TEXT("Game paused, menu shown"));
		}
	}
}

void ALoop9Character::PerformInteractTrace()
{
	if (IsGameplayPresentationLocked())
	{
		return;
	}

	if (!FirstPersonCameraComponent)
	{
		UE_LOG(LogLoop9, Warning, TEXT("FirstPersonCameraComponent is null!"));
		return;
	}

	// Get camera location and forward vector
	FVector CameraLocation = FirstPersonCameraComponent->GetComponentLocation();
	FRotator CameraRotation = FirstPersonCameraComponent->GetComponentRotation();
	FVector ForwardVector = CameraRotation.Vector();

	// Calculate precise line trace start and end points
	FVector TraceStart = CameraLocation;
	FVector TraceEnd = TraceStart + (ForwardVector * InteractMaxDistance);

	// Setup line trace parameters
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true; // Use complex collision for precise hit detection

	// Perform PRECISE LINE TRACE (no sweep, pure raycast)
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	// Draw debug line (optional, for visualization)
	#if WITH_EDITOR
	if (bShowInteractDebug)
	{
		DrawDebugLine(
			GetWorld(),
			TraceStart,
			bHit ? HitResult.Location : TraceEnd,
			bHit ? FColor::Green : FColor::Red,
			false,
			2.0f,
			0,
			2.0f
		);

		// Draw impact point
		if (bHit)
		{
			DrawDebugSphere(
				GetWorld(),
				HitResult.Location,
				5.0f,
				8,
				FColor::Yellow,
				false,
				2.0f
			);
		}
	}
	#endif

	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();
		UE_LOG(LogLoop9, Log, TEXT("Interact: Hit actor '%s'"), *HitActor->GetName());

		if (HitActor->GetClass()->ImplementsInterface(ULoop9Interactable::StaticClass()))
		{
			APlayerController* PC = Cast<APlayerController>(GetController());
			if (ILoop9Interactable::Execute_TryInteract(HitActor, PC))
			{
				UE_LOG(LogLoop9, Log, TEXT("Interacted with actor through interface: %s"), *HitActor->GetClass()->GetName());
				return;
			}
			// A false interface result means "not handled", so an attached
			// InspectableComponent still gets a chance to handle the action.
		}

		// Drop-in inspection: any actor with an InspectableComponent can be
		// examined without implementing the interactable interface.
		if (UInspectableComponent* Inspectable = HitActor->FindComponentByClass<UInspectableComponent>())
		{
			APlayerController* PC = Cast<APlayerController>(GetController());
			Inspectable->StartInspection(PC);
			return;
		}

		// If none of the above, log what we hit
		UE_LOG(LogLoop9, Log, TEXT("Hit actor is not interactable: %s"), *HitActor->GetClass()->GetName());
	}
	else
	{
		UE_LOG(LogLoop9, Log, TEXT("Interact trace hit nothing"));
	}
}

