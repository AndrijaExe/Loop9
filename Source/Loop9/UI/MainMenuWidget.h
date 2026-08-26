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
	void EnsureArchiveButton();
	void EnsureHelpButton();
	void EnsureCreditsButton();

	/**
	 * Clones the Settings button and inserts the copy immediately before Anchor
	 * (Quit if Anchor is null), so a WBP that predates this entry still shows it.
	 * Returns null when there is no Settings button to copy the style from.
	 */
	UWidget* SynthesizeButtonBefore(FName ButtonName, UWidget* Anchor);

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

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnHelpClicked();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnBackFromHelp();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnCreditsClicked();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnBackFromCredits();

	/** Settings Widget Class (set in Blueprint) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> SettingsWidgetClass;

	/** Optional dossier widget. Empty uses the C++ Shift Archive fallback. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> ArchiveWidgetClass;

	/** Optional briefing widget. Empty uses the C++ Help fallback. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> HelpWidgetClass;

	/** Optional credits widget. Empty uses the C++ Credits fallback. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> CreditsWidgetClass;

	/** Back from settings to main menu */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OnBackFromSettings();

private:
	/** Not UPROPERTY — names match WBP_Button variables in the Blueprint. */
	UWidget* Play = nullptr;
	UWidget* Settings = nullptr;
	UWidget* Quit = nullptr;
	UWidget* Archive = nullptr;
	UWidget* Help = nullptr;
	UWidget* Credits = nullptr;

	/** Settings widget instance */
	UPROPERTY()
	UUserWidget* SettingsWidgetInstance;

	UPROPERTY()
	UUserWidget* ArchiveWidgetInstance;

	UPROPERTY()
	UUserWidget* HelpWidgetInstance;

	UPROPERTY()
	UUserWidget* CreditsWidgetInstance;

	/** Cached game mode reference */
	UPROPERTY()
	class AMainMenuGameMode* MainMenuGameMode;

	FDelegateHandle CultureChangedHandle;
};
