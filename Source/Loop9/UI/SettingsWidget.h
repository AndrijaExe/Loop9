// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "SettingsWidget.generated.h"

/**
 * Settings Widget - Base C++ class
 * Create Blueprint child to design settings UI
 * Handle graphics, audio, controls, etc.
 */
UCLASS()
class LOOP9_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleApplyButtonClicked();

	UFUNCTION()
	void HandleBackButtonClicked();

	/** Combo selection only. Culture is applied in OnApplyClicked / SetLanguage. */
	UFUNCTION()
	void HandleLanguageSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleGammaChanged(float Value);

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleAmbientVolumeChanged(float Value);

	UFUNCTION()
	void HandleMouseSensitivityChanged(float Value);

	UFUNCTION()
	void HandleInvertYChanged(bool bIsChecked);

	UFUNCTION()
	void HandleResolutionScaleChanged(float Value);

	UFUNCTION()
	void HandleVSyncChanged(bool bIsChecked);

	/** True while we rebuild localized combo strings — ignores language combo callbacks. */
	bool bIsRefreshingLocalizedUI = false;
	bool bIsSyncingWidgets = false;

	/** Core-ticker handle for the deferred combo refresh. World timers don't
	 *  tick while the game is paused (settings opened from the pause menu),
	 *  so the refresh must go through FTSTicker instead. */
	FTSTicker::FDelegateHandle LanguageRefreshTickerHandle;


public:
	/** Called when Back button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void OnBackClicked();

	/** Called when Apply button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void OnApplyClicked();

	/** Menu that opened this settings screen (MainMenu or PauseMenu). */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetReturnTarget(UUserWidget* InReturnTarget);

	/** Removes every USettingsWidget currently on the viewport (orphans included). */
	static void RemoveAllFromViewport(UWorld* World);

	// --- GRAPHICS SETTINGS ---
	
	/** Set screen resolution */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetResolution(int32 Width, int32 Height);

	/** Set fullscreen mode */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetFullscreenMode(bool bFullscreen);

	/** Set window mode (0=Windowed, 1=Fullscreen, 2=Borderless) */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetWindowMode(int32 WindowModeIndex);

	/** Set V-Sync */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetVSync(bool bEnabled);

	/** Set graphics quality (Low, Medium, High, Epic) */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetGraphicsQuality(int32 QualityLevel);

	/** Set resolution scale (0..1 normalized across engine min/max). */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetResolutionScalePercent(float ScaleNormalized);

	/** Set FPS limit (0 = uncapped) */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetFrameRateLimit(float Limit);

	/** Read current resolution */
	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	void GetCurrentResolution(int32& OutWidth, int32& OutHeight) const;

	/** Read current window mode (0=Windowed, 1=Fullscreen, 2=Borderless) */
	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	int32 GetCurrentWindowMode() const;

	/** Read current VSync */
	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	bool GetCurrentVSyncEnabled() const;

	/** Read current quality */
	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	int32 GetCurrentGraphicsQuality() const;

	/** Read current resolution scale (0..1 normalized). */
	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	float GetCurrentResolutionScalePercent() const;

	/** Read current FPS limit */
	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	float GetCurrentFrameRateLimit() const;

	// --- AUDIO SETTINGS ---

	/** Set master volume */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float Volume);

	/** Set ambient volume */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetAmbientVolume(float Volume);

	// --- DISPLAY SETTINGS ---

	/** Set display gamma (2.2 = neutral, higher = brighter). */
	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void SetGamma(float InGamma);

	/** Read current display gamma. */
	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	float GetCurrentGamma() const;

	// --- CONTROLS SETTINGS ---

	/** Set mouse sensitivity */
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetMouseSensitivity(float Sensitivity);

	/** Set inverted Y-axis */
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetInvertedYAxis(bool bInverted);

	// --- LANGUAGE SETTINGS ---

	/** Set UI language by culture code (e.g. "en", "sr"). Persists across sessions. */
	UFUNCTION(BlueprintCallable, Category = "Settings|Language")
	void SetLanguage(const FString& CultureCode);

	/** Current UI culture code (e.g. "en"). */
	UFUNCTION(BlueprintPure, Category = "Settings|Language")
	FString GetCurrentLanguage() const;

	// --- SAVE/LOAD SETTINGS ---

	/** Save all settings to config file */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettings();

	/** Load settings from config file */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadSettings();

private:
	/** Menu that should be restored when Back is pressed. */
	UPROPERTY()
	TWeakObjectPtr<UUserWidget> ReturnTarget;

	// Standardized row components (WBP_ComboRow / WBP_SliderRow / WBP_CheckRow
	// instances in the designer, named exactly like these properties).

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9ComboRow* ComboBoxString_WindowMode;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9ComboRow* ComboBoxString_Resolution;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9ComboRow* ComboBoxString_Quality;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9CheckRow* CheckBox_VSync;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9SliderRow* Slider_ResolutionScale;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9ComboRow* ComboBoxString_FPSLimit;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9ComboRow* ComboBoxString_Language;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9SliderRow* Slider_Gamma;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9SliderRow* Slider_MasterVolume;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9SliderRow* Slider_AmbientVolume;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9SliderRow* Slider_MouseSensitivity;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class ULoop9CheckRow* CheckBox_InvertY;

	/** Header title text (designer name TextBlock_0). */
	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UTextBlock* TextBlock_0;

	/** Preferred footer buttons (WBP_Button). */
	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UWidget* Btn_Apply;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UWidget* Btn_Back;

	/** Legacy raw buttons (optional). Prefer Btn_Apply / Btn_Back (WBP_Button). */
	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UButton* Button_Apply;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UButton* Button_Back;

	void PopulateGraphicsOptions();
	void PopulateLanguageOptions();
	void BindValueWidgets();
	void SyncWidgetsFromCurrentSettings();
	/** Re-applies labels and rebuilds localized combo option strings (by index). */
	void RefreshLocalizedUI();
	void ApplyLocalizedLabels();
	/** Rebuilds only combo FString options after the language dropdown has closed. */
	UFUNCTION()
	void RefreshLocalizedComboOptions();
	/** Reads current widget values and pushes them into GameUserSettings / game settings. */
	void ApplyWidgetsToSettings();
	class ULoop9GameSettingsSubsystem* GetGameSettings() const;

	/** UI slider range for gamma; slider value 0-1 maps to this range. */
	static constexpr float MinGamma = 1.6f;
	static constexpr float MaxGamma = 3.2f;

	/** UI slider range for mouse sensitivity. */
	static constexpr float MinSensitivity = 0.1f;
	static constexpr float MaxSensitivity = 3.0f;

	/** Culture codes supported by the game, index-aligned with ComboBoxString_Language options. */
	static const TArray<FString> SupportedCultures;
};
