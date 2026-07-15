// Fill out your copyright notice in the Description page of Project Settings.

#include "SettingsWidget.h"
#include "MainMenuWidget.h"
#include "PauseMenuWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/Button.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"

// Index-aligned with the options added in PopulateLanguageOptions().
const TArray<FString> USettingsWidget::SupportedCultures = { TEXT("en"), TEXT("sr") };

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LoadSettings();
   PopulateGraphicsOptions();
	PopulateLanguageOptions();
	BindValueWidgets();
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
	// UComboBoxString works with display strings; selection logic below relies on
	// indices (window mode, quality) or numeric parsing (resolution, FPS), so
	// localized labels are safe here.
	if (ComboBoxString_WindowMode)
	{
		ComboBoxString_WindowMode->ClearOptions();
		ComboBoxString_WindowMode->AddOption(NSLOCTEXT("Loop9Settings", "WindowModeWindowed", "Windowed").ToString());
		ComboBoxString_WindowMode->AddOption(NSLOCTEXT("Loop9Settings", "WindowModeFullscreen", "Fullscreen").ToString());
		ComboBoxString_WindowMode->AddOption(NSLOCTEXT("Loop9Settings", "WindowModeBorderless", "Borderless").ToString());
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
		ComboBoxString_Quality->AddOption(NSLOCTEXT("Loop9Settings", "QualityLow", "Low").ToString());
		ComboBoxString_Quality->AddOption(NSLOCTEXT("Loop9Settings", "QualityMedium", "Medium").ToString());
		ComboBoxString_Quality->AddOption(NSLOCTEXT("Loop9Settings", "QualityHigh", "High").ToString());
		ComboBoxString_Quality->AddOption(NSLOCTEXT("Loop9Settings", "QualityEpic", "Epic").ToString());
	}

	if (ComboBoxString_FPSLimit)
	{
		ComboBoxString_FPSLimit->ClearOptions();
		ComboBoxString_FPSLimit->AddOption(NSLOCTEXT("Loop9Settings", "FPSUncapped", "Uncapped").ToString());
		ComboBoxString_FPSLimit->AddOption(TEXT("30"));
		ComboBoxString_FPSLimit->AddOption(TEXT("60"));
		ComboBoxString_FPSLimit->AddOption(TEXT("120"));
		ComboBoxString_FPSLimit->AddOption(TEXT("144"));
	}
}

void USettingsWidget::PopulateLanguageOptions()
{
	if (!ComboBoxString_Language)
	{
		return;
	}

	ComboBoxString_Language->ClearOptions();
	// Language names are shown in their own language on purpose (standard practice).
	ComboBoxString_Language->AddOption(TEXT("English"));
	ComboBoxString_Language->AddOption(TEXT("Srpski"));

	ComboBoxString_Language->OnSelectionChanged.Clear();
	ComboBoxString_Language->OnSelectionChanged.AddDynamic(this, &USettingsWidget::HandleLanguageSelectionChanged);
}

void USettingsWidget::HandleLanguageSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct || !ComboBoxString_Language)
	{
		return;
	}

	const int32 Index = ComboBoxString_Language->GetSelectedIndex();
	if (SupportedCultures.IsValidIndex(Index))
	{
		SetLanguage(SupportedCultures[Index]);
	}
}

void USettingsWidget::BindValueWidgets()
{
	auto BindSlider = [this](USlider* Slider, void (USettingsWidget::*Handler)(float))
	{
		if (Slider)
		{
			Slider->OnValueChanged.Clear();
			Slider->OnValueChanged.AddDynamic(this, Handler);
		}
	};

	BindSlider(Slider_Gamma, &USettingsWidget::HandleGammaChanged);
	BindSlider(Slider_MasterVolume, &USettingsWidget::HandleMasterVolumeChanged);
	BindSlider(Slider_MusicVolume, &USettingsWidget::HandleMusicVolumeChanged);
	BindSlider(Slider_SFXVolume, &USettingsWidget::HandleSFXVolumeChanged);
	BindSlider(Slider_MouseSensitivity, &USettingsWidget::HandleMouseSensitivityChanged);

	if (CheckBox_InvertY)
	{
		CheckBox_InvertY->OnCheckStateChanged.Clear();
		CheckBox_InvertY->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::HandleInvertYChanged);
	}
}

void USettingsWidget::HandleGammaChanged(float Value)
{
	SetGamma(FMath::Lerp(MinGamma, MaxGamma, FMath::Clamp(Value, 0.0f, 1.0f)));
}

void USettingsWidget::HandleMasterVolumeChanged(float Value)
{
	SetMasterVolume(Value);
}

void USettingsWidget::HandleMusicVolumeChanged(float Value)
{
	SetMusicVolume(Value);
}

void USettingsWidget::HandleSFXVolumeChanged(float Value)
{
	SetSFXVolume(Value);
}

void USettingsWidget::HandleMouseSensitivityChanged(float Value)
{
	SetMouseSensitivity(FMath::Lerp(MinSensitivity, MaxSensitivity, FMath::Clamp(Value, 0.0f, 1.0f)));
}

void USettingsWidget::HandleInvertYChanged(bool bIsChecked)
{
	SetInvertedYAxis(bIsChecked);
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

	if (const ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		if (Slider_Gamma)
		{
			Slider_Gamma->SetValue(FMath::GetRangePct(MinGamma, MaxGamma, GameSettings->GetGamma()));
		}
		if (Slider_MasterVolume)
		{
			Slider_MasterVolume->SetValue(GameSettings->GetMasterVolume());
		}
		if (Slider_MusicVolume)
		{
			Slider_MusicVolume->SetValue(GameSettings->GetMusicVolume());
		}
		if (Slider_SFXVolume)
		{
			Slider_SFXVolume->SetValue(GameSettings->GetSFXVolume());
		}
		if (Slider_MouseSensitivity)
		{
			Slider_MouseSensitivity->SetValue(
				FMath::GetRangePct(MinSensitivity, MaxSensitivity, GameSettings->GetMouseSensitivity()));
		}
		if (CheckBox_InvertY)
		{
			CheckBox_InvertY->SetIsChecked(GameSettings->IsYAxisInverted());
		}
	}

	if (ComboBoxString_Language)
	{
		const FString CurrentCulture = GetCurrentLanguage();
		int32 SelectedIndex = 0;
		for (int32 i = 0; i < SupportedCultures.Num(); ++i)
		{
			// Match "sr" against "sr", "sr-Latn", "sr-RS"...
			if (CurrentCulture.StartsWith(SupportedCultures[i]))
			{
				SelectedIndex = i;
				break;
			}
		}
		ComboBoxString_Language->SetSelectedIndex(SelectedIndex);
	}

	if (ComboBoxString_FPSLimit)
	{
		const float Limit = GetCurrentFrameRateLimit();
		if (Limit <= 0.1f)
		{
			ComboBoxString_FPSLimit->SetSelectedOption(NSLOCTEXT("Loop9Settings", "FPSUncapped", "Uncapped").ToString());
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

ULoop9GameSettingsSubsystem* USettingsWidget::GetGameSettings() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<ULoop9GameSettingsSubsystem>();
	}

	return nullptr;
}

void USettingsWidget::SetMasterVolume(float Volume)
{
	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->SetMasterVolume(Volume);
	}
}

void USettingsWidget::SetMusicVolume(float Volume)
{
	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->SetMusicVolume(Volume);
	}
}

void USettingsWidget::SetSFXVolume(float Volume)
{
	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->SetSFXVolume(Volume);
	}
}

// --- DISPLAY SETTINGS ---

void USettingsWidget::SetGamma(float InGamma)
{
	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->SetGamma(InGamma);
	}
}

float USettingsWidget::GetCurrentGamma() const
{
	if (const ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		return GameSettings->GetGamma();
	}

	return 2.2f;
}

// --- CONTROLS SETTINGS ---

void USettingsWidget::SetMouseSensitivity(float Sensitivity)
{
	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->SetMouseSensitivity(Sensitivity);
	}
}

void USettingsWidget::SetInvertedYAxis(bool bInverted)
{
	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->SetInvertYAxis(bInverted);
	}
}

// --- LANGUAGE SETTINGS ---

void USettingsWidget::SetLanguage(const FString& CultureCode)
{
	// bSaveToConfig persists the culture to GameUserSettings.ini,
	// so the engine restores it automatically on next launch.
	UKismetInternationalizationLibrary::SetCurrentCulture(CultureCode, /*SaveToConfig*/ true);
	UE_LOG(LogTemp, Log, TEXT("SettingsWidget: Language set to '%s'"), *CultureCode);
}

FString USettingsWidget::GetCurrentLanguage() const
{
	return UKismetInternationalizationLibrary::GetCurrentCulture();
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
