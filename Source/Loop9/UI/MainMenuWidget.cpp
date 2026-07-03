// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Get game mode reference
	MainMenuGameMode = Cast<AMainMenuGameMode>(UGameplayStatics::GetGameMode(GetWorld()));

	if (!MainMenuGameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: Failed to get MainMenuGameMode!"));
	}
}

void UMainMenuWidget::OnPlayClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Play button clicked"));

	if (MainMenuGameMode)
	{
		MainMenuGameMode->StartGame();
	}
	else
	{
		// Fallback: directly load level
		UGameplayStatics::OpenLevel(GetWorld(), FName("MainLevel"));
	}
}

void UMainMenuWidget::OnSettingsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Settings button clicked"));

	if (!SettingsWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: SettingsWidgetClass not set!"));
		return;
	}

	// Create settings widget
	SettingsWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), SettingsWidgetClass);

	if (SettingsWidgetInstance)
	{
		// Hide main menu
		SetVisibility(ESlateVisibility::Hidden);

		// Show settings
		SettingsWidgetInstance->AddToViewport(1); // Higher Z-order
		UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Settings widget opened"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: Failed to create settings widget!"));
	}
}

void UMainMenuWidget::OnQuitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Quit button clicked"));

	if (MainMenuGameMode)
	{
		MainMenuGameMode->QuitGame();
	}
	else
	{
		// Fallback: quit directly
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC)
		{
			PC->ConsoleCommand("quit");
		}
	}
}

void UMainMenuWidget::OnBackFromSettings()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Back from settings"));

	// Remove settings widget
	if (SettingsWidgetInstance)
	{
		SettingsWidgetInstance->RemoveFromParent();
		SettingsWidgetInstance = nullptr;
	}

	// Show main menu again
	SetVisibility(ESlateVisibility::Visible);
}
