// Copyright Epic Games, Inc. All Rights Reserved.

#include "Loop9GameMode.h"
#include "Controllers/Loop9PlayerController.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Loop9.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ALoop9GameMode::ALoop9GameMode()
{
	PlayerControllerClass = ALoop9PlayerController::StaticClass();

	static ConstructorHelpers::FObjectFinder<USoundBase> MusicFinder(
		TEXT("/Game/MyStuff/Sound/Ambient/HorrorAmbience1"));
	if (MusicFinder.Succeeded())
	{
		LevelMusicSound = MusicFinder.Object;
	}
}

void ALoop9GameMode::BeginPlay()
{
	Super::BeginPlay();
	StartLevelMusic();
}

void ALoop9GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLevelMusic();
	Super::EndPlay(EndPlayReason);
}

float ALoop9GameMode::ResolveMusicVolume() const
{
	float AmbientMultiplier = 1.0f;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoop9GameSettingsSubsystem* Settings =
				GameInstance->GetSubsystem<ULoop9GameSettingsSubsystem>())
		{
			AmbientMultiplier = Settings->GetAmbientVolume();
		}
	}

	return LevelMusicVolume * AmbientMultiplier;
}

void ALoop9GameMode::StartLevelMusic()
{
	if (bLevelMusicSuppressed || !LevelMusicSound)
	{
		return;
	}

	if (LevelMusicAudioComponent && LevelMusicAudioComponent->IsPlaying())
	{
		return;
	}

	if (!LevelMusicAudioComponent)
	{
		LevelMusicAudioComponent = UGameplayStatics::SpawnSound2D(
			this,
			LevelMusicSound,
			ResolveMusicVolume(),
			1.0f,
			0.0f,
			nullptr,
			/*bPersistAcrossLevelTransition*/ false,
			/*bAutoDestroy*/ false);

		if (!LevelMusicAudioComponent)
		{
			UE_LOG(LogLoop9, Warning, TEXT("Level music: SpawnSound2D returned nothing."));
			return;
		}

		LevelMusicAudioComponent->bAutoDestroy = false;
		LevelMusicAudioComponent->OnAudioFinished.AddDynamic(
			this, &ALoop9GameMode::HandleLevelMusicFinished);
		return;
	}

	LevelMusicAudioComponent->SetVolumeMultiplier(ResolveMusicVolume());
	LevelMusicAudioComponent->Play();
}

void ALoop9GameMode::StopLevelMusic()
{
	if (!LevelMusicAudioComponent)
	{
		return;
	}

	LevelMusicAudioComponent->OnAudioFinished.RemoveDynamic(
		this, &ALoop9GameMode::HandleLevelMusicFinished);
	LevelMusicAudioComponent->Stop();
	LevelMusicAudioComponent->DestroyComponent();
	LevelMusicAudioComponent = nullptr;
}

void ALoop9GameMode::HandleLevelMusicFinished()
{
	if (bLevelMusicSuppressed || !LevelMusicAudioComponent)
	{
		return;
	}

	LevelMusicAudioComponent->Play();
}

bool ALoop9GameMode::IsLevelMusicPlaying() const
{
	return LevelMusicAudioComponent && LevelMusicAudioComponent->IsPlaying();
}

void ALoop9GameMode::ApplyAmbientVolume(float AmbientVolume)
{
	if (LevelMusicAudioComponent)
	{
		LevelMusicAudioComponent->SetVolumeMultiplier(LevelMusicVolume * AmbientVolume);
	}
}

void ALoop9GameMode::EnsureLevelMusicPlaying()
{
	// BeginPlay can land before the world has an audio device, in which case the
	// spawn above produced nothing audible. The settings subsystem calls this once
	// the device is up.
	StartLevelMusic();
}

void ALoop9GameMode::SetMusicSuppressed(bool bSuppressed)
{
	if (bLevelMusicSuppressed == bSuppressed)
	{
		return;
	}

	bLevelMusicSuppressed = bSuppressed;

	if (bSuppressed)
	{
		if (LevelMusicAudioComponent)
		{
			LevelMusicAudioComponent->Stop();
		}
		return;
	}

	StartLevelMusic();
}
