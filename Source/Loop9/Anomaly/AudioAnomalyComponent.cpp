#include "Anomaly/AudioAnomalyComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

UAudioAnomalyComponent::UAudioAnomalyComponent()
{
	AnomalyProbability = 0.5f;
}

bool UAudioAnomalyComponent::ApplyAnomalyState()
{
	if (!AnomalySound)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

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

	return true;
}

void UAudioAnomalyComponent::RestoreNormalState()
{
	if (RuntimeAudioComponent)
	{
		RuntimeAudioComponent->Stop();
		RuntimeAudioComponent->DestroyComponent();
		RuntimeAudioComponent = nullptr;
	}
}
