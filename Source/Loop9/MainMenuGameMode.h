// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

/**
 * GameMode for Main Menu level
 * Handles UI initialization and menu flow
 */
UCLASS()
class LOOP9_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public:
	/** Main Menu Widget Class (set in Blueprint) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	/** Loading Screen Widget Class (optional) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

	/** Optional ambient loop sound for main menu */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> MainMenuLoopSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	float MainMenuLoopVolume = 0.35f;

    /** Camera tag to find menu camera in level (default: MenuCamera) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera")
	FName MenuCameraActorTag = TEXT("MenuCamera");

	/** Enable subtle menu camera drift */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera")
	bool bEnableMenuCameraDrift = true;

	/** Drift speed (lower = slower) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera", meta = (ClampMin = "0.01"))
  float CameraDriftSpeed = 0.35f;

	/** Positional drift amplitude in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera", meta = (ClampMin = "0.0"))
  float CameraDriftLocationAmplitude = 8.0f;

	/** Rotational drift amplitude in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera", meta = (ClampMin = "0.0"))
	float CameraDriftYawAmplitude = 0.4f;

	/** Instance of the main menu widget */
	UPROPERTY()
	UUserWidget* MainMenuWidgetInstance;

	/** Show the main menu */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowMainMenu();

	/** Start the game (load gameplay level) */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartGame();

	/** Open settings menu */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenSettings();

	/** Quit the game */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void QuitGame();

private:
	/** Player controller reference */
	APlayerController* PlayerController;

	FVector MenuCameraBaseLocation = FVector::ZeroVector;
	FRotator MenuCameraBaseRotation = FRotator::ZeroRotator;
	float MenuCameraTime = 0.0f;

	TObjectPtr<class ACameraActor> ResolvedMenuCameraActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> MainMenuAudioComponent = nullptr;
};
