// Fill out your copyright notice in the Description page of Project Settings.

#include "LiftButton.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"

ALiftButton::ALiftButton()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create Static Mesh Component
	ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
	RootComponent = ButtonMesh;

	// Default button type
	ButtonType = ELiftButtonType::Increment;
}

void ALiftButton::BeginPlay()
{
	Super::BeginPlay();
}

void ALiftButton::Interact()
{
	// Get LoopManagerSubsystem
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("LiftButton: GameInstance is null!"));
		return;
	}

	ULoopManagerSubsystem* LoopManager = GameInstance->GetSubsystem<ULoopManagerSubsystem>();
	if (!LoopManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("LiftButton: LoopManagerSubsystem not found!"));
		return;
	}

	// Call appropriate function based on button type
	switch (ButtonType)
	{
		case ELiftButtonType::Increment:
			UE_LOG(LogTemp, Log, TEXT("LiftButton: Increment button pressed"));
			LoopManager->OnElevatorButtonPressed(EButtonType::Increment);
			break;

		case ELiftButtonType::Reset:
			UE_LOG(LogTemp, Log, TEXT("LiftButton: Reset button pressed"));
			LoopManager->OnElevatorButtonPressed(EButtonType::Reset);
			break;

		default:
			break;
	}

	// Trigger Blueprint event for visual feedback (if implemented)
	OnInteracted();
}

bool ALiftButton::TryInteract_Implementation(APlayerController* InteractingController)
{
	Interact();
	return true;
}

FText ALiftButton::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("Use"));
}
