// Fill out your copyright notice in the Description page of Project Settings.

#include "PauseMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameHelpers.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPauseMenuWidget::OnResumeClicked()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Resume clicked"));

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		// Unpause the game
		UGameplayStatics::SetGamePaused(GetWorld(), false);

		// Restore game input mode
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}

	// Remove pause menu
	RemoveFromParent();
}

void UPauseMenuWidget::OnSettingsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Settings clicked"));

	if (!SettingsWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PauseMenuWidget: SettingsWidgetClass not set!"));
		return;
	}

	// Create settings widget
	SettingsWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), SettingsWidgetClass);

	if (SettingsWidgetInstance)
	{
		// Hide pause menu
		SetVisibility(ESlateVisibility::Hidden);

		// Show settings
		SettingsWidgetInstance->AddToViewport(2); // Higher Z-order
		UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Settings opened"));
	}
}

void UPauseMenuWidget::OnMainMenuClicked()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Main Menu clicked"));

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		// Unpause first
		UGameplayStatics::SetGamePaused(GetWorld(), false);
	}

	// Load Main Menu level (correct name: MainMenu)
	UGameplayStatics::OpenLevel(GetWorld(), FName("MainMenu"));
}

void UPauseMenuWidget::OnQuitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Quit clicked"));

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		// Unpause first
		UGameplayStatics::SetGamePaused(GetWorld(), false);

		// Quit the game
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}

void UPauseMenuWidget::OnBackFromSettings()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Back from settings"));

	// Remove settings widget
	if (SettingsWidgetInstance)
	{
		SettingsWidgetInstance->RemoveFromParent();
		SettingsWidgetInstance = nullptr;
	}

	// Show pause menu again
	SetVisibility(ESlateVisibility::Visible);
}
