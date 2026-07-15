// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

/**
 * Main Menu Widget - Base C++ class
 * Create Blueprint child to design UI
 */
UCLASS()
class LOOP9_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void ApplyLocalizedTexts();
	void HandleCultureChanged();

public:
	/** Called when Play button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnPlayClicked();

	/** Called when Settings button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnSettingsClicked();

	/** Called when Quit button is clicked */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnQuitClicked();

	/** Settings Widget Class (set in Blueprint) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> SettingsWidgetClass;

	/** Back from settings to main menu */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnBackFromSettings();

private:
	/** Not UPROPERTY — names match WBP_Button variables in the Blueprint. */
	UWidget* Play = nullptr;
	UWidget* Settings = nullptr;
	UWidget* Quit = nullptr;

	/** Settings widget instance */
	UPROPERTY()
	UUserWidget* SettingsWidgetInstance;

	/** Cached game mode reference */
	UPROPERTY()
	class AMainMenuGameMode* MainMenuGameMode;

	FDelegateHandle CultureChangedHandle;
};
