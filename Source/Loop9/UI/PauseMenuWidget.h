// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

/**
 * Pause Menu Widget - Base C++ class
 * Shows when player presses ESC during gameplay
 */
UCLASS()
class LOOP9_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void ApplyLocalizedTexts();
	void HandleCultureChanged();

public:
	/** Called when Resume button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void OnResumeClicked();

	/** Called when Settings button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void OnSettingsClicked();

	/** Called when Main Menu button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void OnMainMenuClicked();

	/** Called when Quit button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void OnQuitClicked();

	/** Settings Widget Class (set in Blueprint) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> SettingsWidgetClass;

	/** Back from settings to pause menu */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void OnBackFromSettings();

private:
	/** Not UPROPERTY — names match WBP_Button variables in the Blueprint. */
	UWidget* ResumeButton = nullptr;
	UWidget* SettingsButton = nullptr;
	UWidget* MainMenuButton = nullptr;
	UWidget* QuitButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Pause Menu|Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<class UTextBlock> TextBlock_0;

	/** Settings widget instance */
	UPROPERTY()
	UUserWidget* SettingsWidgetInstance;

	FDelegateHandle CultureChangedHandle;
};
