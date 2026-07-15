// Fill out your copyright notice in the Description page of Project Settings.

#include "PauseMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Internationalization/Internationalization.h"
#include "Loop9WidgetClickBinder.h"
#include "SettingsWidget.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ResumeButton)
	{
		ResumeButton = GetWidgetFromName(TEXT("ResumeButton"));
	}
	if (!SettingsButton)
	{
		SettingsButton = GetWidgetFromName(TEXT("SettingsButton"));
	}
	if (!MainMenuButton)
	{
		MainMenuButton = GetWidgetFromName(TEXT("MainMenuButton"));
	}
	if (!QuitButton)
	{
		QuitButton = GetWidgetFromName(TEXT("QuitButton"));
	}
	if (!TextBlock_0)
	{
		TextBlock_0 = Cast<UTextBlock>(GetWidgetFromName(TEXT("TextBlock_0")));
	}

	FLoop9WidgetClickBinder::BindClicked(ResumeButton, this, GET_FUNCTION_NAME_CHECKED(UPauseMenuWidget, OnResumeClicked));
	FLoop9WidgetClickBinder::BindClicked(SettingsButton, this, GET_FUNCTION_NAME_CHECKED(UPauseMenuWidget, OnSettingsClicked));
	FLoop9WidgetClickBinder::BindClicked(MainMenuButton, this, GET_FUNCTION_NAME_CHECKED(UPauseMenuWidget, OnMainMenuClicked));
	FLoop9WidgetClickBinder::BindClicked(QuitButton, this, GET_FUNCTION_NAME_CHECKED(UPauseMenuWidget, OnQuitClicked));

	ApplyLocalizedTexts();

	if (!CultureChangedHandle.IsValid())
	{
		CultureChangedHandle = FInternationalization::Get().OnCultureChanged().AddUObject(
			this, &UPauseMenuWidget::HandleCultureChanged);
	}
}

void UPauseMenuWidget::NativeDestruct()
{
	FLoop9WidgetClickBinder::UnbindClicked(ResumeButton, this, GET_FUNCTION_NAME_CHECKED(UPauseMenuWidget, OnResumeClicked));
	FLoop9WidgetClickBinder::UnbindClicked(SettingsButton, this, GET_FUNCTION_NAME_CHECKED(UPauseMenuWidget, OnSettingsClicked));
	FLoop9WidgetClickBinder::UnbindClicked(MainMenuButton, this, GET_FUNCTION_NAME_CHECKED(UPauseMenuWidget, OnMainMenuClicked));
	FLoop9WidgetClickBinder::UnbindClicked(QuitButton, this, GET_FUNCTION_NAME_CHECKED(UPauseMenuWidget, OnQuitClicked));

	if (CultureChangedHandle.IsValid())
	{
		FInternationalization::Get().OnCultureChanged().Remove(CultureChangedHandle);
		CultureChangedHandle.Reset();
	}

	Super::NativeDestruct();
}

void UPauseMenuWidget::HandleCultureChanged()
{
	ApplyLocalizedTexts();
}

void UPauseMenuWidget::ApplyLocalizedTexts()
{
	if (TextBlock_0)
	{
		TextBlock_0->SetText(NSLOCTEXT("Loop9Menu", "Paused", "PAUSED"));
	}

	FLoop9WidgetClickBinder::SetButtonText(ResumeButton, NSLOCTEXT("Loop9Menu", "Resume", "RESUME"));
	FLoop9WidgetClickBinder::SetButtonText(SettingsButton, NSLOCTEXT("Loop9Menu", "Settings", "SETTINGS"));
	FLoop9WidgetClickBinder::SetButtonText(MainMenuButton, NSLOCTEXT("Loop9Menu", "MainMenu", "MAIN MENU"));
	FLoop9WidgetClickBinder::SetButtonText(QuitButton, NSLOCTEXT("Loop9Menu", "Quit", "QUIT"));
}

void UPauseMenuWidget::OnResumeClicked()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Resume clicked"));

	// Sweep leftover settings / combo popups before returning to gameplay.
	USettingsWidget::RemoveAllFromViewport(GetWorld());
	SettingsWidgetInstance = nullptr;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}

	RemoveFromParent();
}

void UPauseMenuWidget::OnSettingsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Settings clicked"));

	// C++ and Blueprint may both bind the same click — ignore the duplicate.
	if (SettingsWidgetInstance && SettingsWidgetInstance->IsInViewport())
	{
		return;
	}

	if (!SettingsWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PauseMenuWidget: SettingsWidgetClass not set!"));
		return;
	}

	USettingsWidget::RemoveAllFromViewport(GetWorld());

	SettingsWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), SettingsWidgetClass);
	if (SettingsWidgetInstance)
	{
		if (USettingsWidget* Settings = Cast<USettingsWidget>(SettingsWidgetInstance))
		{
			Settings->SetReturnTarget(this);
		}

		SetVisibility(ESlateVisibility::Hidden);
		SettingsWidgetInstance->AddToViewport(2);
		UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Settings opened"));
	}
}

void UPauseMenuWidget::OnMainMenuClicked()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Main Menu clicked"));

	USettingsWidget::RemoveAllFromViewport(GetWorld());
	SettingsWidgetInstance = nullptr;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
	}

	UGameplayStatics::OpenLevel(GetWorld(), FName("MainMenu"));
}

void UPauseMenuWidget::OnQuitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Quit clicked"));

	USettingsWidget::RemoveAllFromViewport(GetWorld());
	SettingsWidgetInstance = nullptr;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}

void UPauseMenuWidget::OnBackFromSettings()
{
	UE_LOG(LogTemp, Log, TEXT("PauseMenuWidget: Back from settings"));

	USettingsWidget::RemoveAllFromViewport(GetWorld());
	SettingsWidgetInstance = nullptr;

	ApplyLocalizedTexts();
	SetVisibility(ESlateVisibility::Visible);
}
