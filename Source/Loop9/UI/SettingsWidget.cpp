// Fill out your copyright notice in the Description page of Project Settings.

#include "SettingsWidget.h"
#include "MainMenuWidget.h"
#include "PauseMenuWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/Button.h"

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LoadSettings();
   PopulateGraphicsOptions();
	SyncWidgetsFromCurrentSettings();

	if (Button_Apply)
	{
		Button_Apply->OnClicked.RemoveDynamic(this, &USettingsWidget::HandleApplyButtonClicked);
		Button_Apply->OnClicked.AddDynamic(this, &USettingsWidget::HandleApplyButtonClicked);
	}

	if (Button_Back)
	{
		Button_Back->OnClicked.RemoveDynamic(this, &USettingsWidget::HandleBackButtonClicked);
		Button_Back->OnClicked.AddDynamic(this, &USettingsWidget::HandleBackButtonClicked);
	}
}

void USettingsWidget::HandleApplyButtonClicked()
{
	OnApplyClicked();
}

void USettingsWidget::HandleBackButtonClicked()
{
	OnBackClicked();
}

void USettingsWidget::PopulateGraphicsOptions()
{
	if (ComboBoxString_WindowMode)
	{
		ComboBoxString_WindowMode->ClearOptions();
		ComboBoxString_WindowMode->AddOption(TEXT("Windowed"));
		ComboBoxString_WindowMode->AddOption(TEXT("Fullscreen"));
		ComboBoxString_WindowMode->AddOption(TEXT("Borderless"));
	}

	if (ComboBoxString_Resolution)
	{
		ComboBoxString_Resolution->ClearOptions();
		ComboBoxString_Resolution->AddOption(TEXT("1280x720"));
		ComboBoxString_Resolution->AddOption(TEXT("1600x900"));
		ComboBoxString_Resolution->AddOption(TEXT("1920x1080"));
		ComboBoxString_Resolution->AddOption(TEXT("2560x1440"));
	}

	if (ComboBoxString_Quality)
	{
		ComboBoxString_Quality->ClearOptions();
		ComboBoxString_Quality->AddOption(TEXT("Low"));
		ComboBoxString_Quality->AddOption(TEXT("Medium"));
		ComboBoxString_Quality->AddOption(TEXT("High"));
		ComboBoxString_Quality->AddOption(TEXT("Epic"));
	}

	if (ComboBoxString_FPSLimit)
	{
		ComboBoxString_FPSLimit->ClearOptions();
		ComboBoxString_FPSLimit->AddOption(TEXT("Uncapped"));
		ComboBoxString_FPSLimit->AddOption(TEXT("30"));
		ComboBoxString_FPSLimit->AddOption(TEXT("60"));
		ComboBoxString_FPSLimit->AddOption(TEXT("120"));
		ComboBoxString_FPSLimit->AddOption(TEXT("144"));
	}
}

void USettingsWidget::SyncWidgetsFromCurrentSettings()
{
	int32 Width = 0;
	int32 Height = 0;
	GetCurrentResolution(Width, Height);

	if (ComboBoxString_WindowMode)
	{
		ComboBoxString_WindowMode->SetSelectedIndex(FMath::Clamp(GetCurrentWindowMode(), 0, 2));
	}

	if (ComboBoxString_Resolution)
	{
		const FString ResString = FString::Printf(TEXT("%dx%d"), Width, Height);
		if (ComboBoxString_Resolution->FindOptionIndex(ResString) != INDEX_NONE)
		{
			ComboBoxString_Resolution->SetSelectedOption(ResString);
		}
		else
		{
			ComboBoxString_Resolution->SetSelectedOption(TEXT("1920x1080"));
		}
	}

	if (ComboBoxString_Quality)
	{
		ComboBoxString_Quality->SetSelectedIndex(FMath::Clamp(GetCurrentGraphicsQuality(), 0, 3));
	}

	if (CheckBox_VSync)
	{
		CheckBox_VSync->SetIsChecked(GetCurrentVSyncEnabled());
	}

	if (Slider_ResolutionScale)
	{
		Slider_ResolutionScale->SetValue(GetCurrentResolutionScalePercent() / 100.0f);
	}

	if (ComboBoxString_FPSLimit)
	{
		const float Limit = GetCurrentFrameRateLimit();
		if (Limit <= 0.1f)
		{
			ComboBoxString_FPSLimit->SetSelectedOption(TEXT("Uncapped"));
		}
		else
		{
			const FString LimitString = FString::Printf(TEXT("%.0f"), Limit);
			if (ComboBoxString_FPSLimit->FindOptionIndex(LimitString) != INDEX_NONE)
			{
				ComboBoxString_FPSLimit->SetSelectedOption(LimitString);
			}
			else
			{
				ComboBoxString_FPSLimit->SetSelectedOption(TEXT("60"));
			}
		}
	}
}

void USettingsWidget::OnBackClicked()
{
	UE_LOG(LogTemp, Log, TEXT("SettingsWidget: Back button clicked"));

	// Find ANY parent menu widget (MainMenu or PauseMenu)
	TArray<UUserWidget*> AllWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), AllWidgets, UUserWidget::StaticClass());

	for (UUserWidget* Widget : AllWidgets)
	{
		// Check if it's MainMenuWidget
		if (UMainMenuWidget* MainMenu = Cast<UMainMenuWidget>(Widget))
		{
			UE_LOG(LogTemp, Log, TEXT("Found MainMenuWidget - returning to it"));
			MainMenu->OnBackFromSettings();
			RemoveFromParent();
			return;
		}

		// Check if it's PauseMenuWidget
		if (UPauseMenuWidget* PauseMenu = Cast<UPauseMenuWidget>(Widget))
		{
			UE_LOG(LogTemp, Log, TEXT("Found PauseMenuWidget - returning to it"));
			PauseMenu->OnBackFromSettings();
			RemoveFromParent();
			return;
		}
	}

	// Fallback: just close settings
	UE_LOG(LogTemp, Warning, TEXT("No parent menu found - just closing settings"));
	RemoveFromParent();
}

void USettingsWidget::OnApplyClicked()
{
	UE_LOG(LogTemp, Log, TEXT("SettingsWidget: Apply button clicked"));
	
	// Save settings
	SaveSettings();
}

// --- GRAPHICS SETTINGS ---

void USettingsWidget::SetResolution(int32 Width, int32 Height)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->SetScreenResolution(FIntPoint(Width, Height));
		UE_LOG(LogTemp, Log, TEXT("Resolution set to: %dx%d"), Width, Height);
	}
}

void USettingsWidget::SetFullscreenMode(bool bFullscreen)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->SetFullscreenMode(bFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed);
	}
}

void USettingsWidget::SetWindowMode(int32 WindowModeIndex)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		return;
	}

	EWindowMode::Type Mode = EWindowMode::Windowed;
	if (WindowModeIndex == 1)
	{
		Mode = EWindowMode::Fullscreen;
	}
	else if (WindowModeIndex == 2)
	{
		Mode = EWindowMode::WindowedFullscreen;
	}

	Settings->SetFullscreenMode(Mode);
}

void USettingsWidget::SetVSync(bool bEnabled)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->SetVSyncEnabled(bEnabled);
		UE_LOG(LogTemp, Log, TEXT("VSync: %s"), bEnabled ? TEXT("ON") : TEXT("OFF"));
	}
}

void USettingsWidget::SetGraphicsQuality(int32 QualityLevel)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		// QualityLevel: 0=Low, 1=Medium, 2=High, 3=Epic
     Settings->SetOverallScalabilityLevel(FMath::Clamp(QualityLevel, 0, 3));
	}
}

void USettingsWidget::SetResolutionScalePercent(float ScalePercent)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->SetResolutionScaleValueEx(FMath::Clamp(ScalePercent, 50.0f, 100.0f));
	}
}

void USettingsWidget::SetFrameRateLimit(float Limit)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->SetFrameRateLimit(FMath::Max(0.0f, Limit));
	}
}

void USettingsWidget::GetCurrentResolution(int32& OutWidth, int32& OutHeight) const
{
	OutWidth = 0;
	OutHeight = 0;

	if (const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		const FIntPoint Res = Settings->GetScreenResolution();
		OutWidth = Res.X;
		OutHeight = Res.Y;
	}
}

int32 USettingsWidget::GetCurrentWindowMode() const
{
	if (const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		switch (Settings->GetFullscreenMode())
		{
		case EWindowMode::Fullscreen:
			return 1;
		case EWindowMode::WindowedFullscreen:
			return 2;
		default:
			return 0;
		}
	}

	return 0;
}

bool USettingsWidget::GetCurrentVSyncEnabled() const
{
	if (const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		return Settings->IsVSyncEnabled();
	}

	return false;
}

int32 USettingsWidget::GetCurrentGraphicsQuality() const
{
	if (const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		return Settings->GetOverallScalabilityLevel();
	}

	return 2;
}

float USettingsWidget::GetCurrentResolutionScalePercent() const
{
	if (const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		return Settings->GetResolutionScaleNormalized() * 100.0f;
	}

	return 100.0f;
}

float USettingsWidget::GetCurrentFrameRateLimit() const
{
	if (const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		return Settings->GetFrameRateLimit();
	}

	return 0.0f;
}

// --- AUDIO SETTINGS ---

void USettingsWidget::SetMasterVolume(float Volume)
{
	// Implement audio volume control here
	// Requires Sound Class setup in Unreal
	UE_LOG(LogTemp, Log, TEXT("Master Volume set to: %.2f"), Volume);
	
	// Example (requires Sound Class setup):
	// USoundClass* MasterClass = LoadObject<USoundClass>(nullptr, TEXT("/Game/Audio/MasterSoundClass"));
	// if (MasterClass) { ... }
}

void USettingsWidget::SetMusicVolume(float Volume)
{
	UE_LOG(LogTemp, Log, TEXT("Music Volume set to: %.2f"), Volume);
	// Implement music volume control
}

void USettingsWidget::SetSFXVolume(float Volume)
{
	UE_LOG(LogTemp, Log, TEXT("SFX Volume set to: %.2f"), Volume);
	// Implement SFX volume control
}

// --- CONTROLS SETTINGS ---

void USettingsWidget::SetMouseSensitivity(float Sensitivity)
{
	// Save to config or custom save system
	UE_LOG(LogTemp, Log, TEXT("Mouse Sensitivity set to: %.2f"), Sensitivity);
	
	// Can be applied to player controller in-game
}

void USettingsWidget::SetInvertedYAxis(bool bInverted)
{
	UE_LOG(LogTemp, Log, TEXT("Inverted Y-Axis: %s"), bInverted ? TEXT("ON") : TEXT("OFF"));
	// Save preference
}

// --- SAVE/LOAD SETTINGS ---

void USettingsWidget::SaveSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
     Settings->ApplyNonResolutionSettings();
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}
}

void USettingsWidget::LoadSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->LoadSettings();
     Settings->ApplyNonResolutionSettings();
	}
}
