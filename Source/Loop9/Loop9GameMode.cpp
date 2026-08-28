// Copyright Epic Games, Inc. All Rights Reserved.

#include "Loop9GameMode.h"
#include "Controllers/Loop9PlayerController.h"
#include "Subsystems/AnomalyManager.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Loop9.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"
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

	static ConstructorHelpers::FObjectFinder<USoundBase> AltMusicFinder(
		TEXT("/Game/MyStuff/Sound/Ambient/726368__christmaskrumble666__street-museum-dark-ambient-bgm"));
	if (AltMusicFinder.Succeeded())
	{
		LevelMusicAltSound = AltMusicFinder.Object;
	}
}

void ALoop9GameMode::BeginPlay()
{
	Super::BeginPlay();
	StartLevelMusic();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (UGameInstance* GameInstance = GetGameInstance())
			{
				if (UAnomalyManager* Manager = GameInstance->GetSubsystem<UAnomalyManager>())
				{
					Manager->EnsureScaleAnomalyPlacement(GetWorld());
				}
			}
		}));
	}
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

USoundBase* ALoop9GameMode::ResolveCurrentLevelMusicTrack() const
{
	if (bPlayingLevelMusicAlt && LevelMusicAltSound)
	{
		return LevelMusicAltSound;
	}
	return LevelMusicSound;
}

float ALoop9GameMode::ResolveTrackDuration(USoundBase* Sound) const
{
	if (!Sound)
	{
		return 0.0f;
	}

	if (const USoundWave* Wave = Cast<USoundWave>(Sound))
	{
		if (Wave->Duration > 0.05f)
		{
			return Wave->Duration;
		}
	}

	const float Duration = Sound->GetDuration();
	return (Duration > 0.05f && Duration < 9000.0f) ? Duration : 0.0f;
}

void ALoop9GameMode::ClearLevelMusicAdvanceTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LevelMusicAdvanceTimerHandle);
	}
}

void ALoop9GameMode::PlayCurrentLevelMusicTrack()
{
	if (bLevelMusicSuppressed)
	{
		return;
	}

	if (!LevelMusicAltSound)
	{
		LevelMusicAltSound = LoadObject<USoundBase>(
			nullptr,
			TEXT("/Game/MyStuff/Sound/Ambient/726368__christmaskrumble666__street-museum-dark-ambient-bgm.726368__christmaskrumble666__street-museum-dark-ambient-bgm"));
	}

	USoundBase* Sound = ResolveCurrentLevelMusicTrack();
	if (!Sound)
	{
		return;
	}

	ClearLevelMusicAdvanceTimer();

	if (!LevelMusicAudioComponent)
	{
		LevelMusicAudioComponent = UGameplayStatics::SpawnSound2D(
			this,
			Sound,
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
	}
	else
	{
		LevelMusicAudioComponent->Stop();
		LevelMusicAudioComponent->SetSound(Sound);
		LevelMusicAudioComponent->SetVolumeMultiplier(ResolveMusicVolume());
		LevelMusicAudioComponent->Play();
	}

	const float Duration = ResolveTrackDuration(Sound);
	if (Duration > 0.05f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				LevelMusicAdvanceTimerHandle,
				this,
				&ALoop9GameMode::AdvanceLevelMusicPlaylist,
				Duration,
				false);
		}
	}
}

void ALoop9GameMode::AdvanceLevelMusicPlaylist()
{
	if (bLevelMusicSuppressed)
	{
		return;
	}

	if (LevelMusicAltSound)
	{
		bPlayingLevelMusicAlt = !bPlayingLevelMusicAlt;
	}

	PlayCurrentLevelMusicTrack();
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

	PlayCurrentLevelMusicTrack();
}

void ALoop9GameMode::StopLevelMusic()
{
	ClearLevelMusicAdvanceTimer();
	bPlayingLevelMusicAlt = false;

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
	if (bLevelMusicSuppressed)
	{
		return;
	}

	// Looping waves never arrive here. A non-looping alt clip does; the timer
	// is also armed, so ignore a finished event if the timer still owns the swap.
	if (UWorld* World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerActive(LevelMusicAdvanceTimerHandle))
		{
			return;
		}
	}

	AdvanceLevelMusicPlaylist();
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
		ClearLevelMusicAdvanceTimer();
		if (LevelMusicAudioComponent)
		{
			LevelMusicAudioComponent->Stop();
		}
		return;
	}

	bPlayingLevelMusicAlt = false;
	PlayCurrentLevelMusicTrack();
}
