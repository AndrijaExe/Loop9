#include "Anomaly/LightFlickerAnomalyComponent.h"

#include "Components/LightComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ULightFlickerAnomalyComponent::ULightFlickerAnomalyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Defaulted so every placed light is audible without per-instance authoring;
	// a designer who wants a silent flicker clears the property.
	static ConstructorHelpers::FObjectFinder<USoundBase> FlickerSoundFinder(
		TEXT("/Game/MyStuff/Sound/MainMenu/light-flicker"));
	if (FlickerSoundFinder.Succeeded())
	{
		FlickerSound = FlickerSoundFinder.Object;
	}
}

void ULightFlickerAnomalyComponent::BeginPlay()
{
	if (AActor* Owner = GetOwner())
	{
		if (!TargetLightComponentName.IsNone())
		{
			for (UActorComponent* Comp : Owner->GetComponents())
			{
				if (Comp && Comp->GetFName() == TargetLightComponentName)
				{
					if (ULightComponent* Light = Cast<ULightComponent>(Comp))
					{
						TargetLights.Add(Light);
					}
				}
			}
		}

		if (TargetLights.Num() == 0)
		{
			TArray<ULightComponent*> FoundLights;
			Owner->GetComponents<ULightComponent>(FoundLights);
			for (ULightComponent* Light : FoundLights)
			{
				if (Light)
				{
					TargetLights.Add(Light);
				}
			}
		}
	}

	BaseIntensities.Empty();
	for (ULightComponent* Light : TargetLights)
	{
		BaseIntensities.Add(Light ? Light->Intensity : 0.0f);
	}

	Super::BeginPlay();
}

void ULightFlickerAnomalyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsAnomalyActive || TargetLights.Num() == 0)
	{
		return;
	}

	FlickerTime += DeltaTime;
	const float Wave = (FMath::Sin(FlickerTime * FlickerSpeed) * 0.5f) + 0.5f;
	const float BaseMult = FMath::Lerp(MinIntensityMultiplier, MaxIntensityMultiplier, Wave);
	const float Jitter = FMath::Lerp(1.0f - RandomJitterStrength, 1.0f + RandomJitterStrength, FMath::FRand());

	for (int32 i = 0; i < TargetLights.Num(); ++i)
	{
		if (ULightComponent* Light = TargetLights[i])
		{
			const float BaseIntensity = BaseIntensities.IsValidIndex(i) ? BaseIntensities[i] : Light->Intensity;
			Light->SetIntensity(BaseIntensity * BaseMult * Jitter);
		}
	}

	TryPlayFlickerSound(DeltaTime, Wave);
}

FVector ULightFlickerAnomalyComponent::GetFlickerSoundLocation() const
{
	// The fitting, not the actor pivot: a ceiling panel's owner origin can sit a
	// room away, and the point of this sound is that it tells the player where
	// to walk.
	for (const TObjectPtr<ULightComponent>& Light : TargetLights)
	{
		if (Light)
		{
			return Light->GetComponentLocation();
		}
	}

	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

void ULightFlickerAnomalyComponent::TryPlayFlickerSound(float DeltaTime, float NormalizedIntensity)
{
	if (!FlickerSound)
	{
		return;
	}

	FlickerSoundCooldown = FMath::Max(FlickerSoundCooldown - DeltaTime, 0.0f);

	const bool bIsBelow = NormalizedIntensity <= FlickerSoundTriggerLevel;
	const bool bJustDipped = bIsBelow && !bWasBelowTriggerLevel;
	bWasBelowTriggerLevel = bIsBelow;

	if (!bJustDipped || FlickerSoundCooldown > 0.0f)
	{
		return;
	}

	FlickerSoundCooldown = MinSecondsBetweenFlickerSounds
		+ FMath::FRandRange(0.0f, FlickerSoundJitterSeconds);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		FlickerSound,
		GetFlickerSoundLocation(),
		FRotator::ZeroRotator,
		FlickerSoundVolume,
		FMath::FRandRange(0.92f, 1.08f),
		0.0f,
		FlickerSoundAttenuation);
}

bool ULightFlickerAnomalyComponent::ApplyAnomalyState()
{
	FlickerTime = 0.0f;
	FlickerSoundCooldown = 0.0f;
	bWasBelowTriggerLevel = false;
	SetComponentTickEnabled(true);
	return true;
}

void ULightFlickerAnomalyComponent::RestoreNormalState()
{
	SetComponentTickEnabled(false);

	for (int32 i = 0; i < TargetLights.Num(); ++i)
	{
		if (ULightComponent* Light = TargetLights[i])
		{
			const float BaseIntensity = BaseIntensities.IsValidIndex(i) ? BaseIntensities[i] : Light->Intensity;
			Light->SetIntensity(BaseIntensity);
		}
	}
}
