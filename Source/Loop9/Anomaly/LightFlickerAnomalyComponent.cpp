#include "Anomaly/LightFlickerAnomalyComponent.h"

#include "Components/LightComponent.h"

ULightFlickerAnomalyComponent::ULightFlickerAnomalyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
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
}

bool ULightFlickerAnomalyComponent::ApplyAnomalyState()
{
	FlickerTime = 0.0f;
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
