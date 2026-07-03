// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Horror/HorrorPlayerController.h"

#include "HorrorCharacter.h"
#include "HorrorUI.h"
#include "Loop9.h"
#include "Loop9CameraManager.h"

AHorrorPlayerController::AHorrorPlayerController()
{
	PlayerCameraManagerClass = ALoop9CameraManager::StaticClass();
}

void AHorrorPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AHorrorPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (AHorrorCharacter* HorrorCharacter = Cast<AHorrorCharacter>(aPawn))
	{
		if (!HorrorUI)
		{
			HorrorUI = CreateWidget<UHorrorUI>(this, HorrorUIClass);
			if (HorrorUI)
			{
				HorrorUI->AddToViewport(0);
			}
		}

		if (HorrorUI)
		{
			HorrorUI->SetupCharacter(HorrorCharacter);
			HorrorUI->SetCrosshairVisible(true);
			ClearInteractionPrompt();
		}
	}
}
UUserWidget* AHorrorPlayerController::GetInteractionPromptWidget() const
{
	return HorrorUI;
}
