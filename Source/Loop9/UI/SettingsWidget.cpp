// Fill out your copyright notice in the Description page of Project Settings.

#include "SettingsWidget.h"
#include "MainMenuWidget.h"
#include "PauseMenuWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Loop9SettingRowWidgets.h"
#include "Loop9WidgetClickBinder.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"

// Index-aligned with the options added in PopulateLanguageOptions().
const TArray<FString> USettingsWidget::SupportedCultures = {
	TEXT("en"), TEXT("sr"), TEXT("de"), TEXT("fr"), TEXT("ru")
};

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LoadSettings();
	PopulateLanguageOptions();
	BindValueWidgets();
	RefreshLocalizedUI();
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

	// Prefer WBP_Button footer bindings from C++ so Back always closes cleanly
	// even if the Blueprint graph is incomplete.
	FLoop9WidgetClickBinder::BindClicked(Btn_Apply, this, GET_FUNCTION_NAME_CHECKED(USettingsWidget, HandleApplyButtonClicked));
	FLoop9WidgetClickBinder::BindClicked(Btn_Back, this, GET_FUNCTION_NAME_CHECKED(USettingsWidget, HandleBackButtonClicked));
}

void USettingsWidget::NativeDestruct()
{
	FTSTicker::GetCoreTicker().RemoveTicker(LanguageRefreshTickerHandle);
	LanguageRefreshTickerHandle.Reset();

	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->FlushPendingSettings();
	}
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->SaveSettings();
	}

	FLoop9WidgetClickBinder::UnbindClicked(Btn_Apply, this, GET_FUNCTION_NAME_CHECKED(USettingsWidget, HandleApplyButtonClicked));
	FLoop9WidgetClickBinder::UnbindClicked(Btn_Back, this, GET_FUNCTION_NAME_CHECKED(USettingsWidget, HandleBackButtonClicked));

	Super::NativeDestruct();
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
		ComboBoxString_Resolution->AddOption(TEXT("1280x800"));
		ComboBoxString_Resolution->AddOption(TEXT("1600x900"));
		ComboBoxString_Resolution->AddOption(TEXT("1920x1080"));
		ComboBoxString_Resolution->AddOption(TEXT("2560x1080"));
		ComboBoxString_Resolution->AddOption(TEXT("2560x1440"));
		ComboBoxString_Resolution->AddOption(TEXT("3440x1440"));
		ComboBoxString_Resolution->AddOption(TEXT("3840x2160"));
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
	ComboBoxString_Language->AddOption(TEXT("Deutsch"));
	ComboBoxString_Language->AddOption(TEXT("Français"));
	ComboBoxString_Language->AddOption(TEXT("Русский"));

	ComboBoxString_Language->OnSelectionChanged.Clear();
	ComboBoxString_Language->OnSelectionChanged.AddDynamic(this, &USettingsWidget::HandleLanguageSelectionChanged);
}

void USettingsWidget::HandleLanguageSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bIsRefreshingLocalizedUI || SelectionType == ESelectInfo::Direct || !ComboBoxString_Language)
	{
		return;
	}

	const int32 Index = ComboBoxString_Language->GetSelectedIndex();
	if (SupportedCultures.IsValidIndex(Index))
	{
		SetLanguage(SupportedCultures[Index]);
	}
}

void USettingsWidget::ApplyLocalizedLabels()
{
	if (TextBlock_0)
	{
		TextBlock_0->SetText(NSLOCTEXT("Loop9Settings", "Title", "SETTINGS"));
	}

	if (ComboBoxString_WindowMode)
	{
		ComboBoxString_WindowMode->SetLabel(NSLOCTEXT("Loop9Settings", "WindowMode", "Window Mode"));
	}
	if (ComboBoxString_Resolution)
	{
		ComboBoxString_Resolution->SetLabel(NSLOCTEXT("Loop9Settings", "Resolution", "Resolution"));
	}
	if (ComboBoxString_Quality)
	{
		ComboBoxString_Quality->SetLabel(NSLOCTEXT("Loop9Settings", "GraphicsQuality", "Graphics Quality"));
	}
	if (Slider_ResolutionScale)
	{
		Slider_ResolutionScale->SetLabel(NSLOCTEXT("Loop9Settings", "ResolutionScale", "Resolution Scale"));
	}
	if (ComboBoxString_FPSLimit)
	{
		ComboBoxString_FPSLimit->SetLabel(NSLOCTEXT("Loop9Settings", "FPSLimit", "FPS Limit"));
	}
	if (CheckBox_VSync)
	{
		CheckBox_VSync->SetLabel(NSLOCTEXT("Loop9Settings", "VSync", "VSync"));
	}
	if (Slider_Gamma)
	{
		Slider_Gamma->SetLabel(NSLOCTEXT("Loop9Settings", "Brightness", "Brightness (Gamma)"));
	}
	if (ComboBoxString_Language)
	{
		ComboBoxString_Language->SetLabel(NSLOCTEXT("Loop9Settings", "Language", "Language"));
	}
	if (Slider_MasterVolume)
	{
		Slider_MasterVolume->SetLabel(NSLOCTEXT("Loop9Settings", "MasterVolume", "Master Volume"));
	}
	if (Slider_AmbientVolume)
	{
		Slider_AmbientVolume->SetLabel(NSLOCTEXT("Loop9Settings", "AmbientVolume", "Ambient Volume"));
	}
	if (Slider_MouseSensitivity)
	{
		Slider_MouseSensitivity->SetLabel(NSLOCTEXT("Loop9Settings", "MouseSensitivity", "Mouse Sensitivity"));
	}
	if (CheckBox_InvertY)
	{
		CheckBox_InvertY->SetLabel(NSLOCTEXT("Loop9Settings", "InvertY", "Invert Y Axis"));
	}

	FLoop9WidgetClickBinder::SetButtonText(Btn_Apply, NSLOCTEXT("Loop9Settings", "Apply", "APPLY"));
	FLoop9WidgetClickBinder::SetButtonText(Btn_Back, NSLOCTEXT("Loop9Settings", "Back", "BACK"));
}

void USettingsWidget::RefreshLocalizedUI()
{
	ApplyLocalizedLabels();
	RefreshLocalizedComboOptions();
}

void USettingsWidget::RefreshLocalizedComboOptions()
{
	const TGuardValue<bool> RefreshGuard(bIsRefreshingLocalizedUI, true);

	// Dropdown from the language combo must already be closed (we defer this
	// call). Sweep any leftover popup layers, then rebuild option strings.
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().DismissAllMenus();
		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
	}

	const int32 WindowModeIdx = ComboBoxString_WindowMode ? ComboBoxString_WindowMode->GetSelectedIndex() : INDEX_NONE;
	const int32 ResolutionIdx = ComboBoxString_Resolution ? ComboBoxString_Resolution->GetSelectedIndex() : INDEX_NONE;
	const int32 QualityIdx = ComboBoxString_Quality ? ComboBoxString_Quality->GetSelectedIndex() : INDEX_NONE;
	const int32 FpsIdx = ComboBoxString_FPSLimit ? ComboBoxString_FPSLimit->GetSelectedIndex() : INDEX_NONE;
	const int32 LanguageIdx = ComboBoxString_Language ? ComboBoxString_Language->GetSelectedIndex() : INDEX_NONE;

	if (ComboBoxString_Language)
	{
		ComboBoxString_Language->OnSelectionChanged.Clear();
	}

	PopulateGraphicsOptions();

	if (ComboBoxString_WindowMode && WindowModeIdx != INDEX_NONE)
	{
		ComboBoxString_WindowMode->SetSelectedIndex(WindowModeIdx);
	}
	if (ComboBoxString_Resolution && ResolutionIdx != INDEX_NONE)
	{
		ComboBoxString_Resolution->SetSelectedIndex(ResolutionIdx);
	}
	if (ComboBoxString_Quality && QualityIdx != INDEX_NONE)
	{
		ComboBoxString_Quality->SetSelectedIndex(QualityIdx);
	}
	if (ComboBoxString_FPSLimit && FpsIdx != INDEX_NONE)
	{
		ComboBoxString_FPSLimit->SetSelectedIndex(FpsIdx);
	}
	if (ComboBoxString_Language)
	{
		if (LanguageIdx != INDEX_NONE)
		{
			ComboBoxString_Language->SetSelectedIndex(LanguageIdx);
		}
		ComboBoxString_Language->OnSelectionChanged.AddDynamic(
			this, &USettingsWidget::HandleLanguageSelectionChanged);
	}
}

void USettingsWidget::BindValueWidgets()
{
	// WBP may still use the old Music row name until the designer renames it.
	if (!Slider_AmbientVolume)
	{
		Slider_AmbientVolume = Cast<ULoop9SliderRow>(GetWidgetFromName(TEXT("Slider_MusicVolume")));
	}
	// SFX volume was removed; hide any leftover row in older WBP_Settings assets.
	if (UWidget* LegacySfx = GetWidgetFromName(TEXT("Slider_SFXVolume")))
	{
		LegacySfx->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Slider_Gamma)
	{
		Slider_Gamma->OnValueChanged.Clear();
		Slider_Gamma->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleGammaChanged);
	}
	if (Slider_MasterVolume)
	{
		Slider_MasterVolume->OnValueChanged.Clear();
		Slider_MasterVolume->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleMasterVolumeChanged);
	}
	if (Slider_AmbientVolume)
	{
		Slider_AmbientVolume->OnValueChanged.Clear();
		Slider_AmbientVolume->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleAmbientVolumeChanged);
	}
	if (Slider_MouseSensitivity)
	{
		Slider_MouseSensitivity->OnValueChanged.Clear();
		Slider_MouseSensitivity->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleMouseSensitivityChanged);
	}
	if (Slider_ResolutionScale)
	{
		Slider_ResolutionScale->OnValueChanged.Clear();
		Slider_ResolutionScale->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleResolutionScaleChanged);
	}

	if (CheckBox_InvertY)
	{
		CheckBox_InvertY->OnCheckStateChanged.Clear();
		CheckBox_InvertY->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::HandleInvertYChanged);
	}
	if (CheckBox_VSync)
	{
		CheckBox_VSync->OnCheckStateChanged.Clear();
		CheckBox_VSync->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::HandleVSyncChanged);
	}
}

void USettingsWidget::HandleGammaChanged(float Value)
{
	if (bIsSyncingWidgets)
	{
		return;
	}
	SetGamma(FMath::Lerp(MinGamma, MaxGamma, FMath::Clamp(Value, 0.0f, 1.0f)));
}

void USettingsWidget::HandleMasterVolumeChanged(float Value)
{
	if (bIsSyncingWidgets)
	{
		return;
	}
	SetMasterVolume(Value);
}

void USettingsWidget::HandleAmbientVolumeChanged(float Value)
{
	if (bIsSyncingWidgets)
	{
		return;
	}
	SetAmbientVolume(Value);
}

void USettingsWidget::HandleMouseSensitivityChanged(float Value)
{
	if (bIsSyncingWidgets)
	{
		return;
	}
	SetMouseSensitivity(FMath::Lerp(MinSensitivity, MaxSensitivity, FMath::Clamp(Value, 0.0f, 1.0f)));
}

void USettingsWidget::HandleInvertYChanged(bool bIsChecked)
{
	if (bIsSyncingWidgets)
	{
		return;
	}
	SetInvertedYAxis(bIsChecked);
}

void USettingsWidget::HandleResolutionScaleChanged(float Value)
{
	if (bIsSyncingWidgets)
	{
		return;
	}
	// Slider is 0..1 normalized across the engine's min/max resolution scale range.
	SetResolutionScalePercent(FMath::Clamp(Value, 0.0f, 1.0f));
}

void USettingsWidget::HandleVSyncChanged(bool bIsChecked)
{
	if (bIsSyncingWidgets)
	{
		return;
	}
	SetVSync(bIsChecked);
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->ApplyNonResolutionSettings();
	}
}

void USettingsWidget::SyncWidgetsFromCurrentSettings()
{
	const TGuardValue<bool> SyncGuard(bIsSyncingWidgets, true);

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
		int32 Quality = GetCurrentGraphicsQuality();
		if (const ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
		{
			// Prefer our persisted choice — engine overall level is often -1 (custom).
			Quality = GameSettings->GetPreferredGraphicsQuality();
		}
		ComboBoxString_Quality->SetSelectedIndex(FMath::Clamp(Quality, 0, 3));
	}

	if (CheckBox_VSync)
	{
		CheckBox_VSync->SetIsChecked(GetCurrentVSyncEnabled());
	}

	if (Slider_ResolutionScale)
	{
		Slider_ResolutionScale->SetValue(GetCurrentResolutionScalePercent());
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
		if (Slider_AmbientVolume)
		{
			Slider_AmbientVolume->SetValue(GameSettings->GetAmbientVolume());
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

void USettingsWidget::SetReturnTarget(UUserWidget* InReturnTarget)
{
	ReturnTarget = InReturnTarget;
}

void USettingsWidget::RemoveAllFromViewport(UWorld* World)
{
	if (!World)
	{
		return;
	}

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().DismissAllMenus();
	}

	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, USettingsWidget::StaticClass(), /*TopLevelOnly*/ false);
	for (UUserWidget* Widget : Found)
	{
		if (Widget && Widget->IsInViewport())
		{
			Widget->RemoveFromParent();
		}
	}
}

void USettingsWidget::OnBackClicked()
{
	UE_LOG(LogTemp, Log, TEXT("SettingsWidget: Back button clicked"));

	SaveSettings();

	FTSTicker::GetCoreTicker().RemoveTicker(LanguageRefreshTickerHandle);
	LanguageRefreshTickerHandle.Reset();

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().DismissAllMenus();
	}

	if (UUserWidget* Target = ReturnTarget.Get())
	{
		if (UPauseMenuWidget* PauseMenu = Cast<UPauseMenuWidget>(Target))
		{
			PauseMenu->OnBackFromSettings();
		}
		else if (UMainMenuWidget* MainMenu = Cast<UMainMenuWidget>(Target))
		{
			MainMenu->OnBackFromSettings();
		}
	}
	else
	{
		// Fallback: prefer an active pause menu over main menu (pause can coexist
		// with a leftover main-menu widget in edge cases).
		TArray<UUserWidget*> AllWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), AllWidgets, UUserWidget::StaticClass());

		UPauseMenuWidget* FoundPause = nullptr;
		UMainMenuWidget* FoundMain = nullptr;
		for (UUserWidget* Widget : AllWidgets)
		{
			if (!FoundPause)
			{
				FoundPause = Cast<UPauseMenuWidget>(Widget);
			}
			if (!FoundMain)
			{
				FoundMain = Cast<UMainMenuWidget>(Widget);
			}
		}

		if (FoundPause)
		{
			FoundPause->OnBackFromSettings();
		}
		else if (FoundMain)
		{
			FoundMain->OnBackFromSettings();
		}
	}

	// Always sweep — covers orphans from double-open (C++ + BP both bound).
	RemoveAllFromViewport(GetWorld());
}

void USettingsWidget::OnApplyClicked()
{
	UE_LOG(LogTemp, Log, TEXT("SettingsWidget: Apply button clicked"));
	ApplyWidgetsToSettings();
	SaveSettings();
}

void USettingsWidget::ApplyWidgetsToSettings()
{
	if (ComboBoxString_WindowMode)
	{
		SetWindowMode(ComboBoxString_WindowMode->GetSelectedIndex());
	}

	if (ComboBoxString_Resolution)
	{
		const FString Res = ComboBoxString_Resolution->GetSelectedOption();
		FString WidthStr;
		FString HeightStr;
		if (Res.Split(TEXT("x"), &WidthStr, &HeightStr))
		{
			SetResolution(FCString::Atoi(*WidthStr), FCString::Atoi(*HeightStr));
		}
	}

	if (ComboBoxString_Quality)
	{
		const int32 Quality = ComboBoxString_Quality->GetSelectedIndex();
		SetGraphicsQuality(Quality);
		if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
		{
			GameSettings->SetPreferredGraphicsQuality(Quality);
		}
	}

	if (CheckBox_VSync)
	{
		SetVSync(CheckBox_VSync->IsChecked());
	}

	if (Slider_ResolutionScale)
	{
		// Slider stores 0..1 normalized scale (engine min..max), not raw percent.
		SetResolutionScalePercent(Slider_ResolutionScale->GetValue());
	}

	if (ComboBoxString_FPSLimit)
	{
		const FString Selected = ComboBoxString_FPSLimit->GetSelectedOption();
		const FString Uncapped = NSLOCTEXT("Loop9Settings", "FPSUncapped", "Uncapped").ToString();
		if (Selected.Equals(Uncapped, ESearchCase::IgnoreCase) || Selected.IsEmpty())
		{
			SetFrameRateLimit(0.0f);
		}
		else
		{
			SetFrameRateLimit(FCString::Atof(*Selected));
		}
	}

	if (ComboBoxString_Language)
	{
		const int32 LangIndex = ComboBoxString_Language->GetSelectedIndex();
		if (SupportedCultures.IsValidIndex(LangIndex))
		{
			SetLanguage(SupportedCultures[LangIndex]);
		}
	}
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
		Settings->ApplyNonResolutionSettings();
		UE_LOG(LogTemp, Log, TEXT("VSync: %s"), bEnabled ? TEXT("ON") : TEXT("OFF"));
	}
}

void USettingsWidget::SetGraphicsQuality(int32 QualityLevel)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		// QualityLevel: 0=Low, 1=Medium, 2=High, 3=Epic
		const int32 Clamped = FMath::Clamp(QualityLevel, 0, 3);
		Settings->SetOverallScalabilityLevel(Clamped);
		UE_LOG(LogTemp, Log, TEXT("Graphics quality set to: %d"), Clamped);
	}

	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->SetPreferredGraphicsQuality(QualityLevel);
	}
}

void USettingsWidget::SetResolutionScalePercent(float ScaleNormalized)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		// Expects 0..1 across Scalability::MinResolutionScale..MaxResolutionScale.
		Settings->SetResolutionScaleNormalized(FMath::Clamp(ScaleNormalized, 0.0f, 1.0f));
		Settings->ApplyNonResolutionSettings();
		UE_LOG(LogTemp, Log, TEXT("Resolution scale normalized: %.2f"), ScaleNormalized);
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
	if (const ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		return GameSettings->GetPreferredGraphicsQuality();
	}

	if (const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		const int32 Overall = Settings->GetOverallScalabilityLevel();
		if (Overall >= 0)
		{
			return Overall;
		}
		// -1 means mixed/custom scalability — fall back to view distance as a proxy.
		return Settings->GetViewDistanceQuality();
	}

	return 2;
}

float USettingsWidget::GetCurrentResolutionScalePercent() const
{
	if (const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		// Normalized 0..1 — matches the slider and SetResolutionScaleNormalized.
		return Settings->GetResolutionScaleNormalized();
	}

	return 1.0f;
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

void USettingsWidget::SetAmbientVolume(float Volume)
{
	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->SetAmbientVolume(Volume);
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
	if (bIsRefreshingLocalizedUI)
	{
		return;
	}

	const bool bOk = UKismetInternationalizationLibrary::SetCurrentCulture(CultureCode, /*SaveToConfig*/ true);
	UE_LOG(LogTemp, Log, TEXT("SettingsWidget: Language set to '%s' (ok=%d)"), *CultureCode, bOk ? 1 : 0);

	// Labels/buttons can update immediately (FText).
	ApplyLocalizedLabels();

	// Do NOT rebuild combo options in the same stack as OnSelectionChanged —
	// the language dropdown is still open and mutating it orphans the popup.
	// Deferred via the core ticker (NOT a world timer): world timers don't
	// tick while the game is paused, and settings opened from the pause menu
	// run with the game paused — a world timer would never fire there.
	FTSTicker::GetCoreTicker().RemoveTicker(LanguageRefreshTickerHandle);
	LanguageRefreshTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			LanguageRefreshTickerHandle.Reset();
			RefreshLocalizedComboOptions();
			return false; // one-shot
		}));
}

FString USettingsWidget::GetCurrentLanguage() const
{
	return UKismetInternationalizationLibrary::GetCurrentCulture();
}

// --- SAVE/LOAD SETTINGS ---

void USettingsWidget::SaveSettings()
{
	if (ULoop9GameSettingsSubsystem* GameSettings = GetGameSettings())
	{
		GameSettings->FlushPendingSettings();
	}

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
