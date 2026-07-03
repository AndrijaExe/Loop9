#include "AudioAnomalyComponent.h"
#include "Subsystems/AnomalyManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

UAudioAnomalyComponent::UAudioAnomalyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAudioAnomalyComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		if (UAnomalyManager* Manager = GI->GetSubsystem<UAnomalyManager>())
		{
			Manager->RegisterAnomaly(GetOwner());
		}
	}
}

void UAudioAnomalyComponent::ActivateAnomaly()
{
	if (bIsAnomalyActive)
	{
		return;
	}

	bIsAnomalyActive = true;
	if (!AnomalySound)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

 // Always respawn based on current mode/options (2D vs spatial),
	// so editor checkbox changes are applied immediately.
	if (RuntimeAudioComponent)
	{
		RuntimeAudioComponent->Stop();
		RuntimeAudioComponent->DestroyComponent();
		RuntimeAudioComponent = nullptr;
	}

	if (bPlaySoundAtLocation)
	{
        if (bFollowOwnerWhenSpatial && Owner->GetRootComponent())
		{
         RuntimeAudioComponent = UGameplayStatics::SpawnSoundAttached(
				AnomalySound,
              Owner->GetRootComponent(),
				NAME_None,
				FVector::ZeroVector,
				EAttachLocation::KeepRelativeOffset,
				false,
				VolumeMultiplier,
				PitchMultiplier,
				0.0f,
                AttenuationSettings,
				nullptr,
				true);
		}
		else
		{
         RuntimeAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
				GetWorld(),
				AnomalySound,
               Owner->GetActorLocation(),
				Owner->GetActorRotation(),
				VolumeMultiplier,
				PitchMultiplier,
				0.0f,
                AttenuationSettings,
				nullptr,
				true);
		}
	}
	else
	{
      RuntimeAudioComponent = UGameplayStatics::SpawnSound2D(
			GetWorld(),
			AnomalySound,
			VolumeMultiplier,
			PitchMultiplier,
			0.0f,
			nullptr,
			true);
	}

	if (RuntimeAudioComponent)
	{
		RuntimeAudioComponent->bIsUISound = false;
        RuntimeAudioComponent->bAllowSpatialization = bPlaySoundAtLocation;
		RuntimeAudioComponent->bAutoDestroy = !bLooping;
		RuntimeAudioComponent->SetVolumeMultiplier(VolumeMultiplier);
		RuntimeAudioComponent->SetPitchMultiplier(PitchMultiplier);
	}
}

void UAudioAnomalyComponent::DeactivateAnomaly()
{
	if (!bIsAnomalyActive)
	{
		return;
	}

	bIsAnomalyActive = false;

	if (RuntimeAudioComponent)
	{
		RuntimeAudioComponent->Stop();
      RuntimeAudioComponent->DestroyComponent();
		RuntimeAudioComponent = nullptr;
	}
}
