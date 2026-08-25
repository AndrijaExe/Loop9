#include "Subsystems/Loop9GameSettingsSubsystem.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Loop9GameMode.h"
#include "MainMenuGameMode.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace Loop9Audio
{
	static const TCHAR* DefaultAmbientSoundClassPath = TEXT("/Game/MyStuff/Sound/SC_Music.SC_Music");
}

void ULoop9GameSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	VolumeSoundMix = NewObject<USoundMix>(this, TEXT("Loop9VolumeMix"));

	ResolveAmbientSoundClass();
	ApplyMasterVolume();
	ApplyGamma();
	ApplySoundClassVolumes(ResolveAudioWorld());

	PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(
		this, &ULoop9GameSettingsSubsystem::HandlePostWorldInit);
}

void ULoop9GameSettingsSubsystem::Deinitialize()
{
	FlushPendingSettings();
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitHandle);

	if (AudioBootstrapTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(AudioBootstrapTickerHandle);
		AudioBootstrapTickerHandle.Reset();
	}

	if (bVolumeMixPushed && VolumeMixWorld.IsValid() && VolumeSoundMix)
	{
		UGameplayStatics::PopSoundMixModifier(VolumeMixWorld.Get(), VolumeSoundMix);
		bVolumeMixPushed = false;
		VolumeMixWorld.Reset();
	}

	Super::Deinitialize();
}

void ULoop9GameSettingsSubsystem::ResolveAmbientSoundClass()
{
	// AmbientSoundClassPath lives on a Config=GameUserSettings class, so the path
	// in DefaultGame.ini is NOT auto-loaded. Read GGameIni explicitly, then fall back.
	if (!AmbientSoundClassPath.IsValid() && GConfig)
	{
		FString PathStr;
		if (GConfig->GetString(
				TEXT("/Script/Loop9.Loop9GameSettingsSubsystem"),
				TEXT("AmbientSoundClassPath"),
				PathStr,
				GGameIni)
			&& !PathStr.IsEmpty())
		{
			AmbientSoundClassPath = FSoftObjectPath(PathStr);
		}
	}

	if (!AmbientSoundClassPath.IsValid())
	{
		AmbientSoundClassPath = FSoftObjectPath(Loop9Audio::DefaultAmbientSoundClassPath);
	}

	AmbientSoundClass = Cast<USoundClass>(AmbientSoundClassPath.TryLoad());
	if (!AmbientSoundClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Loop9 settings: AmbientSoundClass failed to load from '%s'"),
			*AmbientSoundClassPath.ToString());
	}
}

void ULoop9GameSettingsSubsystem::HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues Values)
{
	if (World && World->IsGameWorld() && World->GetGameInstance() == GetGameInstance())
	{
		if (bVolumeMixPushed && (!VolumeMixWorld.IsValid() || VolumeMixWorld.Get() != World))
		{
			if (VolumeMixWorld.IsValid() && VolumeSoundMix)
			{
				UGameplayStatics::PopSoundMixModifier(VolumeMixWorld.Get(), VolumeSoundMix);
			}
			bVolumeMixPushed = false;
			VolumeMixWorld.Reset();
		}

		ApplyMasterVolume();
		ApplySoundClassVolumes(World);

		LevelAmbienceSuppressCount = 0;

		// The world is not running yet, so try again once it is: actors have not
		// had BeginPlay, and the audio device usually is not attached.
		if (AudioBootstrapTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(AudioBootstrapTickerHandle);
		}

		AudioBootstrapAttemptsLeft = 10;
		AudioBootstrapTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(
				this, &ULoop9GameSettingsSubsystem::HandleDeferredAudioBootstrap),
			0.5f);
	}
}

bool ULoop9GameSettingsSubsystem::HandleDeferredAudioBootstrap(float)
{
	--AudioBootstrapAttemptsLeft;

	UWorld* World = ResolveAudioWorld();

	// Keep asking until the world can actually route audio. A single attempt was
	// enough to hide the bug rather than fix it: whenever the device attached late,
	// nothing re-ran and the floor stayed silent until the settings slider moved.
	const bool bAudioReady = World && World->GetAudioDeviceRaw() != nullptr;
	if (!bAudioReady && AudioBootstrapAttemptsLeft > 0)
	{
		return true;
	}

	AudioBootstrapTickerHandle.Reset();

	if (!World)
	{
		return false;
	}

	if (!bAudioReady)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Loop9 audio: gave up waiting for an audio device; the floor will be silent"));
		return false;
	}

	ApplyMasterVolume();
	ApplySoundClassVolumes(World);
	EnsureLevelAmbienceIsPlaying(World);
	return false;
}

bool ULoop9GameSettingsSubsystem::IsMusicAmbience(const UAudioComponent* Audio) const
{
	if (!Audio || !Audio->Sound || !AmbientSoundClass)
	{
		return false;
	}

	const USoundClass* Resolved = Audio->SoundClassOverride
		? Audio->SoundClassOverride.Get()
		: Audio->Sound->SoundClassObject.Get();

	return Resolved == AmbientSoundClass;
}

void ULoop9GameSettingsSubsystem::EnsureLevelAmbienceIsPlaying(UWorld* World) const
{
	if (!World || LevelAmbienceSuppressCount > 0)
	{
		return;
	}

	// The game mode plays the music bed itself, so a map-placed loop on the music
	// sound class would double the track rather than rescue it. Silence those and
	// leave the rest of the placed ambience alone.
	ALoop9GameMode* MusicOwner = ResolveMusicOwningGameMode(World);
	if (MusicOwner)
	{
		MusicOwner->EnsureLevelMusicPlaying();
	}

	for (TActorIterator<AAmbientSound> It(World); It; ++It)
	{
		UAudioComponent* Audio = It->GetAudioComponent();
		if (!Audio || !Audio->Sound)
		{
			continue;
		}

		if (MusicOwner && IsMusicAmbience(Audio))
		{
			if (Audio->IsPlaying())
			{
				Audio->Stop();
				UE_LOG(LogTemp, Log,
					TEXT("Loop9 audio: silenced map-placed music '%s'; the game mode owns the bed"),
					*It->GetActorNameOrLabel());
			}
			continue;
		}

		if (Audio->IsPlaying())
		{
			continue;
		}

		Audio->Play();
		UE_LOG(LogTemp, Log, TEXT("Loop9 audio: restarted level ambience '%s'"),
			*It->GetActorNameOrLabel());
	}
}

ALoop9GameMode* ULoop9GameSettingsSubsystem::ResolveMusicOwningGameMode(UWorld* World) const
{
	ALoop9GameMode* GameMode = World ? Cast<ALoop9GameMode>(World->GetAuthGameMode()) : nullptr;
	return (GameMode && GameMode->LevelMusicSound) ? GameMode : nullptr;
}

void ULoop9GameSettingsSubsystem::SuppressLevelAmbience(bool bSuppress)
{
	if (bSuppress)
	{
		++LevelAmbienceSuppressCount;
	}
	else
	{
		LevelAmbienceSuppressCount = FMath::Max(0, LevelAmbienceSuppressCount - 1);
	}

	ApplyLevelAmbienceSuppressState(ResolveAudioWorld());
}

void ULoop9GameSettingsSubsystem::ApplyLevelAmbienceSuppressState(UWorld* World) const
{
	if (!World)
	{
		return;
	}

	const bool bPause = LevelAmbienceSuppressCount > 0;
	ALoop9GameMode* MusicOwner = ResolveMusicOwningGameMode(World);

	if (MusicOwner)
	{
		MusicOwner->SetMusicSuppressed(bPause);
	}

	for (TActorIterator<AAmbientSound> It(World); It; ++It)
	{
		UAudioComponent* Audio = It->GetAudioComponent();
		if (!Audio || !Audio->Sound)
		{
			continue;
		}

		if (bPause)
		{
			if (Audio->IsPlaying())
			{
				Audio->Stop();
			}
		}
		else if (!Audio->IsPlaying() && !(MusicOwner && IsMusicAmbience(Audio)))
		{
			Audio->Play();
		}
	}
}

UWorld* ULoop9GameSettingsSubsystem::ResolveAudioWorld() const
{
	if (UWorld* World = GetWorld())
	{
		return World;
	}

	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* World = Context.World())
			{
				if (World->IsGameWorld())
				{
					return World;
				}
			}
		}
	}

	return nullptr;
}

// --- Audio ---

void ULoop9GameSettingsSubsystem::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyMasterVolume();
	SchedulePersistence();
}

void ULoop9GameSettingsSubsystem::SetAmbientVolume(float Volume)
{
	AmbientVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplySoundClassVolumes(ResolveAudioWorld());

	if (UWorld* World = ResolveAudioWorld())
	{
		AGameModeBase* GameMode = World->GetAuthGameMode();
		if (AMainMenuGameMode* MainMenu = Cast<AMainMenuGameMode>(GameMode))
		{
			MainMenu->ApplyAmbientVolume(AmbientVolume);
		}
		else if (ALoop9GameMode* Gameplay = Cast<ALoop9GameMode>(GameMode))
		{
			Gameplay->ApplyAmbientVolume(AmbientVolume);
		}
	}

	SchedulePersistence();
}

void ULoop9GameSettingsSubsystem::ApplyMasterVolume() const
{
	FApp::SetVolumeMultiplier(MasterVolume);

	if (GEngine)
	{
		FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice();
		if (AudioDevice.IsValid())
		{
			AudioDevice->SetTransientPrimaryVolume(MasterVolume);
		}
	}
}

void ULoop9GameSettingsSubsystem::ApplySoundClassVolumes(UWorld* World)
{
	if (!AmbientSoundClass)
	{
		ResolveAmbientSoundClass();
	}

	if (!AmbientSoundClass || !World || !VolumeSoundMix)
	{
		return;
	}

	// Prefer sound-mix overrides — never mutate USoundClass asset defaults in-place.
	if (bVolumeMixPushed && (!VolumeMixWorld.IsValid() || VolumeMixWorld.Get() != World))
	{
		if (VolumeMixWorld.IsValid())
		{
			UGameplayStatics::PopSoundMixModifier(VolumeMixWorld.Get(), VolumeSoundMix);
		}
		bVolumeMixPushed = false;
		VolumeMixWorld.Reset();
	}

	// Only claim the push happened once the world can actually route audio.
	// PushSoundMixModifier resolves the device off the world and returns without
	// complaint when there is none, so recording success unconditionally left the
	// mix permanently "pushed" but never active, and every ambient loop on this
	// sound class inaudible for the rest of the level.
	if (World->GetAudioDeviceRaw() == nullptr)
	{
		return;
	}

	if (!bVolumeMixPushed)
	{
		UGameplayStatics::PushSoundMixModifier(World, VolumeSoundMix);
		bVolumeMixPushed = true;
		VolumeMixWorld = World;
	}

	UGameplayStatics::SetSoundMixClassOverride(
		World,
		VolumeSoundMix,
		AmbientSoundClass,
		AmbientVolume,
		/*Pitch*/ 1.0f,
		/*FadeInTime*/ 0.05f,
		/*bApplyToChildren*/ true);
}

void ULoop9GameSettingsSubsystem::SetPreferredGraphicsQuality(int32 QualityLevel)
{
	PreferredGraphicsQuality = FMath::Clamp(QualityLevel, 0, 3);
	SchedulePersistence();
}

// --- Display ---

void ULoop9GameSettingsSubsystem::SetGamma(float InGamma)
{
	Gamma = FMath::Clamp(InGamma, 1.0f, 4.0f);
	ApplyGamma();
	SchedulePersistence();
}

void ULoop9GameSettingsSubsystem::ApplyGamma() const
{
	if (GEngine)
	{
		GEngine->DisplayGamma = Gamma;
	}
}

// --- Controls ---

void ULoop9GameSettingsSubsystem::SetMouseSensitivity(float Sensitivity)
{
	MouseSensitivity = FMath::Clamp(Sensitivity, 0.1f, 5.0f);
	SchedulePersistence();
}

void ULoop9GameSettingsSubsystem::SetInvertYAxis(bool bInverted)
{
	bInvertYAxis = bInverted;
	SchedulePersistence();
}

void ULoop9GameSettingsSubsystem::SchedulePersistence()
{
	bSettingsDirty = true;

	if (PersistenceTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PersistenceTickerHandle);
		PersistenceTickerHandle.Reset();
	}

	PersistenceTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(
			this, &ULoop9GameSettingsSubsystem::HandlePersistenceTicker),
		0.35f);
}

bool ULoop9GameSettingsSubsystem::HandlePersistenceTicker(float)
{
	PersistenceTickerHandle.Reset();
	FlushPendingSettings();
	return false;
}

void ULoop9GameSettingsSubsystem::FlushPendingSettings()
{
	if (PersistenceTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PersistenceTickerHandle);
		PersistenceTickerHandle.Reset();
	}

	if (bSettingsDirty)
	{
		SaveConfig();
		bSettingsDirty = false;
	}
}
