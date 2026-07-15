#include "Anomaly/ClockAnomalyComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

UClockAnomalyComponent::UClockAnomalyComponent()
{
	AnomalyProbability = 0.5f;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UClockAnomalyComponent::BeginPlay()
{
	MinuteHand = ResolveHand(MinuteHandComponentName);
	HourHand = ResolveHand(HourHandComponentName);

	if (MinuteHand)
	{
		MinuteHandNormalRotation = MinuteHand->GetRelativeRotation();
	}

	if (HourHand)
	{
		HourHandNormalRotation = HourHand->GetRelativeRotation();
	}

	Super::BeginPlay();
}

USceneComponent* UClockAnomalyComponent::ResolveHand(FName ComponentName) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<USceneComponent*> Components;
	Owner->GetComponents<USceneComponent>(Components);

	for (USceneComponent* Component : Components)
	{
		if (Component && Component->GetFName() == ComponentName)
		{
			return Component;
		}
	}

	return nullptr;
}

FRotator UClockAnomalyComponent::MakeHandRotation(float Degrees) const
{
	switch (RotationAxis)
	{
	case EClockHandAxis::Pitch: return FRotator(Degrees, 0.0f, 0.0f);
	case EClockHandAxis::Yaw: return FRotator(0.0f, Degrees, 0.0f);
	case EClockHandAxis::Roll:
	default: return FRotator(0.0f, 0.0f, Degrees);
	}
}

bool UClockAnomalyComponent::ApplyAnomalyState()
{
	// Without at least the minute hand there is nothing the player could spot.
	if (!MinuteHand && !HourHand)
	{
		return false;
	}

	SetComponentTickEnabled(true);
	return true;
}

void UClockAnomalyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsAnomalyActive)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (MinuteHand)
	{
		MinuteHand->AddLocalRotation(MakeHandRotation(MinuteHandDegreesPerSecond * DeltaTime));
	}

	if (HourHand)
	{
		HourHand->AddLocalRotation(MakeHandRotation(HourHandDegreesPerSecond * DeltaTime));
	}
}

void UClockAnomalyComponent::RestoreNormalState()
{
	SetComponentTickEnabled(false);

	if (MinuteHand)
	{
		MinuteHand->SetRelativeRotation(MinuteHandNormalRotation);
	}

	if (HourHand)
	{
		HourHand->SetRelativeRotation(HourHandNormalRotation);
	}
}
