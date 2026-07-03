#include "Anomaly/DoorLockStateAnomalyComponent.h"

#include "Interaction/DoorInteractable.h"

UDoorLockStateAnomalyComponent::UDoorLockStateAnomalyComponent()
{
	AnomalyProbability = 0.5f;
}

void UDoorLockStateAnomalyComponent::BeginPlay()
{
	DoorRef = Cast<ADoorInteractable>(GetOwner());
	if (DoorRef)
	{
		bOriginalLockedState = DoorRef->bIsLocked;
		bHasCapturedOriginal = true;
	}

	Super::BeginPlay();
}

bool UDoorLockStateAnomalyComponent::ApplyAnomalyState()
{
	if (!DoorRef)
	{
		return false;
	}

	if (!bHasCapturedOriginal)
	{
		bOriginalLockedState = DoorRef->bIsLocked;
		bHasCapturedOriginal = true;
	}

	DoorRef->SetLocked(!DoorRef->bIsLocked);
	return true;
}

void UDoorLockStateAnomalyComponent::RestoreNormalState()
{
	if (!DoorRef || !bHasCapturedOriginal)
	{
		return;
	}

	DoorRef->SetLocked(bOriginalLockedState);
}
