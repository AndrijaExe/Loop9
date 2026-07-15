// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameHelpers.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

AMainMenuGameMode::AMainMenuGameMode()
{
  PrimaryActorTick.bCanEverTick = true;

	// Disable player pawn and controller for menu
	DefaultPawnClass = nullptr;
	
	// Default player controller
	PlayerControllerClass = APlayerController::StaticClass();
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);

	// Get player controller
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	
	if (PlayerController)
	{
		// Show mouse cursor
		PlayerController->bShowMouseCursor = true;
		PlayerController->bEnableClickEvents = true;
		PlayerController->bEnableMouseOverEvents = true;

		// Set input mode to UI only
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);

        // Resolve camera by tag from current MainMenu level.
		ResolvedMenuCameraActor = nullptr;
		TArray<AActor*> FoundCameras;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), FoundCameras);
		for (AActor* Actor : FoundCameras)
		{
			if (Actor && Actor->ActorHasTag(MenuCameraActorTag))
			{
				ResolvedMenuCameraActor = Cast<ACameraActor>(Actor);
				break;
			}
		}

		// Fallback to first camera if tag not found.
		if (!ResolvedMenuCameraActor && FoundCameras.Num() > 0)
		{
			ResolvedMenuCameraActor = Cast<ACameraActor>(FoundCameras[0]);
		}

		if (ResolvedMenuCameraActor)
		{
            if (USceneComponent* RootComp = ResolvedMenuCameraActor->GetRootComponent())
			{
				if (RootComp->Mobility != EComponentMobility::Movable)
				{
					RootComp->SetMobility(EComponentMobility::Movable);
				}
			}

            PlayerController->SetViewTargetWithBlend(ResolvedMenuCameraActor, 0.0f);
			MenuCameraBaseLocation = ResolvedMenuCameraActor->GetActorLocation();
			MenuCameraBaseRotation = ResolvedMenuCameraActor->GetActorRotation();
			MenuCameraTime = 0.0f;
		}
	}

	// Show main menu
	ShowMainMenu();

	if (MainMenuLoopSound)
	{
		float AmbientMul = 1.0f;
		if (UGameInstance* GI = GetGameInstance())
		{
			if (ULoop9GameSettingsSubsystem* GameSettings = GI->GetSubsystem<ULoop9GameSettingsSubsystem>())
			{
				AmbientMul = GameSettings->GetAmbientVolume();
			}
		}

		MainMenuAudioComponent = UGameplayStatics::SpawnSound2D(
			GetWorld(),
			MainMenuLoopSound,
			MainMenuLoopVolume * AmbientMul,
			1.0f,
			0.0f,
			nullptr,
			true);

		if (MainMenuAudioComponent)
		{
			MainMenuAudioComponent->bAutoDestroy = false;
		}
	}
}

void AMainMenuGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

    if (!bEnableMenuCameraDrift || !ResolvedMenuCameraActor)
	{
		return;
	}

	MenuCameraTime += DeltaSeconds;

	const float T = MenuCameraTime * CameraDriftSpeed;
	const FVector DriftOffset(
		FMath::Sin(T) * CameraDriftLocationAmplitude,
		FMath::Cos(T * 0.75f) * (CameraDriftLocationAmplitude * 0.5f),
		FMath::Sin(T * 0.5f) * (CameraDriftLocationAmplitude * 0.3f));

	const float YawOffset = FMath::Sin(T * 0.8f) * CameraDriftYawAmplitude;

    ResolvedMenuCameraActor->SetActorLocation(MenuCameraBaseLocation + DriftOffset);
	ResolvedMenuCameraActor->SetActorRotation(MenuCameraBaseRotation + FRotator(0.0f, YawOffset, 0.0f));
}

void AMainMenuGameMode::ShowMainMenu()
{
	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuGameMode: MainMenuWidgetClass not set!"));
		return;
	}

	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuGameMode: PlayerController is null!"));
		return;
	}

	// Create main menu widget
	MainMenuWidgetInstance = CreateWidget<UUserWidget>(PlayerController, MainMenuWidgetClass);
	
	if (MainMenuWidgetInstance)
	{
		MainMenuWidgetInstance->AddToViewport();
		UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Main menu shown successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuGameMode: Failed to create main menu widget!"));
	}
}

void AMainMenuGameMode::StartGame()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Starting game..."));

	if (MainMenuAudioComponent)
	{
		MainMenuAudioComponent->Stop();
		MainMenuAudioComponent->DestroyComponent();
		MainMenuAudioComponent = nullptr;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>())
		{
			LoopManager->ResetRunState();
		}
	}

	// Load the main gameplay level with optional loading screen
	if (LoadingScreenWidgetClass)
	{
		UGameHelpers::LoadLevelWithLoadingScreen(GetWorld(), FName("FullOfficeMap"), LoadingScreenWidgetClass);
	}
	else
	{
		UGameplayStatics::OpenLevel(GetWorld(), FName("FullOfficeMap"));
	}
}

void AMainMenuGameMode::OpenSettings()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Opening settings..."));
	
	// Settings will be handled by Blueprint widget
	// This function is exposed for Blueprint to call if needed
}

void AMainMenuGameMode::QuitGame()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Quitting game..."));

	if (MainMenuAudioComponent)
	{
		MainMenuAudioComponent->Stop();
		MainMenuAudioComponent->DestroyComponent();
		MainMenuAudioComponent = nullptr;
	}

	if (PlayerController)
	{
		// Quit the game
		UKismetSystemLibrary::QuitGame(GetWorld(), PlayerController, EQuitPreference::Quit, false);
	}
}

void AMainMenuGameMode::ApplyAmbientVolume(float AmbientVolume)
{
	if (MainMenuAudioComponent)
	{
		MainMenuAudioComponent->SetVolumeMultiplier(MainMenuLoopVolume * FMath::Clamp(AmbientVolume, 0.0f, 1.0f));
	}
}
