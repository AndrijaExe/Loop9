#include "Subsystems/Loop9GameSettingsSubsystem.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void ULoop9GameSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Sound mix that carries the per-class volume overrides.
	VolumeSoundMix = NewObject<USoundMix>(this, TEXT("Loop9VolumeMix"));

	if (MusicSoundClassPath.IsValid())
	{
		MusicSoundClass = Cast<USoundClass>(MusicSoundClassPath.TryLoad());
	}

	if (SFXSoundClassPath.IsValid())
	{
		SFXSoundClass = Cast<USoundClass>(SFXSoundClassPath.TryLoad());
	}

	ApplyMasterVolume();
	ApplyGamma();

	// Sound mix modifiers live per audio device / world, so re-apply them
	// whenever a new game world comes up (level change, main menu -> game...).
	PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(
		this, &ULoop9GameSettingsSubsystem::HandlePostWorldInit);
}

void ULoop9GameSettingsSubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitHandle);
	Super::Deinitialize();
}

void ULoop9GameSettingsSubsystem::HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues Values)
{
	if (World && World->IsGameWorld() && World->GetGameInstance() == GetGameInstance())
	{
		ApplySoundClassVolumes(World);
	}
}

// --- Audio ---

void ULoop9GameSettingsSubsystem::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyMasterVolume();
	PersistSettings();
}

void ULoop9GameSettingsSubsystem::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplySoundClassVolumes(GetWorld());
	PersistSettings();
}

void ULoop9GameSettingsSubsystem::SetSFXVolume(float Volume)
{
	SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplySoundClassVolumes(GetWorld());
	PersistSettings();
}

void ULoop9GameSettingsSubsystem::ApplyMasterVolume() const
{
	// Application-level multiplier: affects every sound without needing any
	// sound class setup on the content side.
	FApp::SetVolumeMultiplier(MasterVolume);
}

void ULoop9GameSettingsSubsystem::ApplySoundClassVolumes(UWorld* World)
{
	if (!World || !VolumeSoundMix)
	{
		return;
	}

	bool bAnyOverride = false;

	if (MusicSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(World, VolumeSoundMix, MusicSoundClass,
			MusicVolume, /*Pitch*/ 1.0f, /*FadeIn*/ 0.2f, /*bApplyToChildren*/ true);
		bAnyOverride = true;
	}

	if (SFXSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(World, VolumeSoundMix, SFXSoundClass,
			SFXVolume, /*Pitch*/ 1.0f, /*FadeIn*/ 0.2f, /*bApplyToChildren*/ true);
		bAnyOverride = true;
	}

	if (bAnyOverride)
	{
		UGameplayStatics::PushSoundMixModifier(World, VolumeSoundMix);
	}
}

// --- Display ---

void ULoop9GameSettingsSubsystem::SetGamma(float InGamma)
{
	Gamma = FMath::Clamp(InGamma, 1.0f, 4.0f);
	ApplyGamma();
	PersistSettings();
}

void ULoop9GameSettingsSubsystem::ApplyGamma() const
{
	if (GEngine)
	{
		// Same mechanism as the "gamma" console command.
		GEngine->DisplayGamma = Gamma;
	}
}

// --- Controls ---

void ULoop9GameSettingsSubsystem::SetMouseSensitivity(float Sensitivity)
{
	MouseSensitivity = FMath::Clamp(Sensitivity, 0.1f, 5.0f);
	PersistSettings();
}

void ULoop9GameSettingsSubsystem::SetInvertYAxis(bool bInverted)
{
	bInvertYAxis = bInverted;
	PersistSettings();
}

void ULoop9GameSettingsSubsystem::PersistSettings()
{
	SaveConfig();
}
