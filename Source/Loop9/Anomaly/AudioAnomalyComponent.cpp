#include "Anomaly/AudioAnomalyComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "UObject/ConstructorHelpers.h"
#include "AI_Friend.h"
#include "Engine/World.h"
#include "Subsystems/Loop9RingingFloorSubsystem.h"
#include "Engine/GameInstance.h"
#include "Subsystems/AnomalyManager.h"

UAudioAnomalyComponent::UAudioAnomalyComponent()
{
	AnomalyProbability = 0.5f;

	static ConstructorHelpers::FObjectFinder<USoundAttenuation> PhoneAttenuationFinder(
		TEXT("/Game/MyStuff/Sound/Phone/ATT_PhoneRinging"));
	if (PhoneAttenuationFinder.Succeeded())
	{
		AttenuationSettings = PhoneAttenuationFinder.Object;
	}
}

void UAudioAnomalyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreNormalState();
	Super::EndPlay(EndPlayReason);
}

bool UAudioAnomalyComponent::ApplyAnomalyState()
{
	if (!AnomalySound)
	{
		return false;
	}
	bAnswered = false;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// 1.1: a phone rings once per run. Refusing here makes the manager draw
	// another anomaly instead of ringing the same desk twice.
	UAnomalyManager* Manager = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UAnomalyManager>()
		: nullptr;
	const bool bIsDeskPhone = bAnswerable && Owner->IsA<AAI_Friend>();
	if (bIsDeskPhone && Manager && Manager->HasPhoneRang(Owner))
	{
		return false;
	}

	if (IsValid(RuntimeAudioComponent))
	{
		RuntimeAudioComponent->Stop();
		RuntimeAudioComponent->DestroyComponent();
	}
	RuntimeAudioComponent = nullptr;

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
			false);
	}

	if (IsValid(RuntimeAudioComponent))
	{
		RuntimeAudioComponent->SetUISound(false);
		RuntimeAudioComponent->bAllowSpatialization = bPlaySoundAtLocation;
		// This component owns the pointer and tears it down explicitly.
		RuntimeAudioComponent->bAutoDestroy = false;
		RuntimeAudioComponent->SetVolumeMultiplier(VolumeMultiplier);
		RuntimeAudioComponent->SetPitchMultiplier(PitchMultiplier);
	}

	// 1.1: a desk phone that rings is a whole beat, not just a sound.
	if (bAnswerable && Owner->IsA<AAI_Friend>())
	{
		if (Manager)
		{
			Manager->MarkPhoneRang(Owner);
		}
		if (ULoop9RingingFloorSubsystem* RingingFloor = GetWorld() ? GetWorld()->GetSubsystem<ULoop9RingingFloorSubsystem>() : nullptr)
		{
			RingingFloor->BeginRingingFloor(Cast<AAI_Friend>(Owner));
		}
	}

	return true;
}

void UAudioAnomalyComponent::RestoreNormalState()
{
	if (bAnswerable)
	{
		if (const AAI_Friend* Phone = Cast<AAI_Friend>(GetOwner()))
		{
			if (ULoop9RingingFloorSubsystem* RingingFloor = GetWorld() ? GetWorld()->GetSubsystem<ULoop9RingingFloorSubsystem>() : nullptr)
			{
				RingingFloor->EndRingingFloor(Phone);
			}
		}
	}
	if (IsValid(RuntimeAudioComponent))
	{
		RuntimeAudioComponent->Stop();
		RuntimeAudioComponent->DestroyComponent();
	}
	RuntimeAudioComponent = nullptr;
	bAnswered = false;
}

bool UAudioAnomalyComponent::IsRinging() const
{
	return bIsAnomalyActive && !bAnswered && IsValid(RuntimeAudioComponent);
}

void UAudioAnomalyComponent::Answer()
{
	bAnswered = true;
	if (IsValid(RuntimeAudioComponent))
	{
		RuntimeAudioComponent->Stop();
		RuntimeAudioComponent->DestroyComponent();
	}
	RuntimeAudioComponent = nullptr;
}
