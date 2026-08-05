#include "Subsystems/Loop9GameSettingsSubsystem.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MainMenuGameMode.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
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
		if (AMainMenuGameMode* MainMenu = Cast<AMainMenuGameMode>(World->GetAuthGameMode()))
		{
			MainMenu->ApplyAmbientVolume(AmbientVolume);
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
