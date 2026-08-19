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

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnArchiveClicked();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnBackFromArchive();

	/** Settings Widget Class (set in Blueprint) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> SettingsWidgetClass;

	/** Optional dossier widget. Empty uses the C++ Shift Archive fallback. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> ArchiveWidgetClass;

	/** Back from settings to main menu */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnBackFromSettings();

private:
	/** Not UPROPERTY — names match WBP_Button variables in the Blueprint. */
	UWidget* Play = nullptr;
	UWidget* Settings = nullptr;
	UWidget* Quit = nullptr;
	UWidget* Archive = nullptr;

	/** Settings widget instance */
	UPROPERTY()
	UUserWidget* SettingsWidgetInstance;

	UPROPERTY()
	UUserWidget* ArchiveWidgetInstance;

	/** Cached game mode reference */
	UPROPERTY()
	class AMainMenuGameMode* MainMenuGameMode;

	FDelegateHandle CultureChangedHandle;
};
