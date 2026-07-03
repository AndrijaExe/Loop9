// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

	UFUNCTION()
	void HandleApplyButtonClicked();

	UFUNCTION()
	void HandleBackButtonClicked();

public:
	/** Called when Back button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void OnBackClicked();

	/** Called when Apply button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void OnApplyClicked();

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

	/** Set resolution scale percent (50-100) */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetResolutionScalePercent(float ScalePercent);

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

	/** Read current resolution scale percent */
	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	float GetCurrentResolutionScalePercent() const;

	/** Read current FPS limit */
	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	float GetCurrentFrameRateLimit() const;

	// --- AUDIO SETTINGS ---

	/** Set master volume */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float Volume);

	/** Set music volume */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float Volume);

	/** Set SFX volume */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float Volume);

	// --- CONTROLS SETTINGS ---

	/** Set mouse sensitivity */
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetMouseSensitivity(float Sensitivity);

	/** Set inverted Y-axis */
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetInvertedYAxis(bool bInverted);

	// --- SAVE/LOAD SETTINGS ---

	/** Save all settings to config file */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettings();

	/** Load settings from config file */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadSettings();

private:
	/** Parent menu widget reference */
	UPROPERTY()
	class UMainMenuWidget* ParentMenuWidget;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UComboBoxString* ComboBoxString_WindowMode;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UComboBoxString* ComboBoxString_Resolution;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UComboBoxString* ComboBoxString_Quality;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UCheckBox* CheckBox_VSync;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class USlider* Slider_ResolutionScale;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UComboBoxString* ComboBoxString_FPSLimit;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UButton* Button_Apply;

  UPROPERTY(BlueprintReadOnly, Category = "Settings|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	class UButton* Button_Back;

	void PopulateGraphicsOptions();
	void SyncWidgetsFromCurrentSettings();
};
