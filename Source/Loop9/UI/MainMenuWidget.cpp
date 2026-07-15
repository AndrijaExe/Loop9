// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Internationalization.h"
#include "Loop9WidgetClickBinder.h"
#include "SettingsWidget.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	MainMenuGameMode = Cast<AMainMenuGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!MainMenuGameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: Failed to get MainMenuGameMode!"));
	}

	if (!Play)
	{
		Play = GetWidgetFromName(TEXT("Play"));
	}
	if (!Settings)
	{
		Settings = GetWidgetFromName(TEXT("Settings"));
	}
	if (!Quit)
	{
		Quit = GetWidgetFromName(TEXT("Quit"));
	}

	FLoop9WidgetClickBinder::BindClicked(Play, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnPlayClicked));
	FLoop9WidgetClickBinder::BindClicked(Settings, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnSettingsClicked));
	FLoop9WidgetClickBinder::BindClicked(Quit, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnQuitClicked));

	ApplyLocalizedTexts();

	if (!CultureChangedHandle.IsValid())
	{
		CultureChangedHandle = FInternationalization::Get().OnCultureChanged().AddUObject(
			this, &UMainMenuWidget::HandleCultureChanged);
	}
}

void UMainMenuWidget::NativeDestruct()
{
	FLoop9WidgetClickBinder::UnbindClicked(Play, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnPlayClicked));
	FLoop9WidgetClickBinder::UnbindClicked(Settings, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnSettingsClicked));
	FLoop9WidgetClickBinder::UnbindClicked(Quit, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnQuitClicked));

	if (CultureChangedHandle.IsValid())
	{
		FInternationalization::Get().OnCultureChanged().Remove(CultureChangedHandle);
		CultureChangedHandle.Reset();
	}

	Super::NativeDestruct();
}

void UMainMenuWidget::HandleCultureChanged()
{
	ApplyLocalizedTexts();
}

void UMainMenuWidget::ApplyLocalizedTexts()
{
	FLoop9WidgetClickBinder::SetButtonText(Play, NSLOCTEXT("Loop9Menu", "Play", "PLAY"));
	FLoop9WidgetClickBinder::SetButtonText(Settings, NSLOCTEXT("Loop9Menu", "Settings", "SETTINGS"));
	FLoop9WidgetClickBinder::SetButtonText(Quit, NSLOCTEXT("Loop9Menu", "Quit", "QUIT"));
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
		UGameplayStatics::OpenLevel(GetWorld(), FName("MainLevel"));
	}
}

void UMainMenuWidget::OnSettingsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Settings button clicked"));

	// C++ and Blueprint may both bind the same WBP_Button click — ignore the duplicate.
	if (SettingsWidgetInstance && SettingsWidgetInstance->IsInViewport())
	{
		return;
	}

	if (!SettingsWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: SettingsWidgetClass not set!"));
		return;
	}

	USettingsWidget::RemoveAllFromViewport(GetWorld());

	SettingsWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), SettingsWidgetClass);
	if (SettingsWidgetInstance)
	{
		if (USettingsWidget* SettingsUI = Cast<USettingsWidget>(SettingsWidgetInstance))
		{
			SettingsUI->SetReturnTarget(this);
		}

		SetVisibility(ESlateVisibility::Hidden);
		SettingsWidgetInstance->AddToViewport(1);
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

	USettingsWidget::RemoveAllFromViewport(GetWorld());
	SettingsWidgetInstance = nullptr;

	ApplyLocalizedTexts();
	SetVisibility(ESlateVisibility::Visible);
}
