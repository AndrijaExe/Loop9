// Copyright Epic Games, Inc. All Rights Reserved.

#include "Loop9PlayerController.h"

#include "Loop9.h"
#include "Loop9CameraManager.h"
#include "UI/BlinkOverlayWidget.h"
#include "Variant_Horror/HorrorCharacter.h"
#include "Variant_Horror/UI/HorrorUI.h"

ALoop9PlayerController::ALoop9PlayerController()
{
	PlayerCameraManagerClass = ALoop9CameraManager::StaticClass();
}

void ALoop9PlayerController::BeginPlay()
{
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetIgnoreLookInput(false);

	Super::BeginPlay();
}

void ALoop9PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (GameplayUIClass)
	{
		if (!GameplayUI)
		{
			GameplayUI = CreateWidget<UHorrorUI>(this, GameplayUIClass);
			if (GameplayUI)
			{
				GameplayUI->AddToViewport(0);
			}
		}

		if (GameplayUI)
		{
			if (AHorrorCharacter* HorrorCharacter = Cast<AHorrorCharacter>(InPawn))
			{
				GameplayUI->SetupCharacter(HorrorCharacter);
			}

			GameplayUI->SetCrosshairVisible(true);
			ClearInteractionPrompt();
		}
	}
}

void ALoop9PlayerController::CreateBlinkOverlay()
{
	if (BlinkOverlayInstance)
	{
		return;
	}

	if (!BlinkOverlayWidgetClass)
	{
		UE_LOG(LogLoop9, Warning, TEXT("BlinkOverlayWidgetClass is not set on Loop9PlayerController."));
		return;
	}

	BlinkOverlayInstance = CreateWidget<UBlinkOverlayWidget>(this, BlinkOverlayWidgetClass);
	if (BlinkOverlayInstance)
	{
		BlinkOverlayInstance->AddToViewport(5000);
	}
}

void ALoop9PlayerController::PlayBlink(float Duration)
{
	if (!BlinkOverlayInstance)
	{
		CreateBlinkOverlay();
	}

	if (BlinkOverlayInstance)
	{
		BlinkOverlayInstance->PlayBlink(Duration);
	}
}

void ALoop9PlayerController::RemoveBlinkOverlay()
{
	if (BlinkOverlayInstance)
	{
		BlinkOverlayInstance->RemoveFromParent();
		BlinkOverlayInstance = nullptr;
	}
}

UHorrorUI* ALoop9PlayerController::GetInteractionPromptUI() const
{
	return GameplayUI;
}
