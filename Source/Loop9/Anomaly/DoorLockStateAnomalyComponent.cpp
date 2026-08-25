#include "Anomaly/DoorLockStateAnomalyComponent.h"

#include "Interaction/DoorInteractable.h"
#include "Loop9.h"

UDoorLockStateAnomalyComponent::UDoorLockStateAnomalyComponent()
{
	AnomalyProbability = 0.5f;
	AnomalyObjectKind = TEXT("a door");
}

void UDoorLockStateAnomalyComponent::BeginPlay()
{
	DoorRef = Cast<ADoorInteractable>(GetOwner());
	if (DoorRef)
	{
		bOriginalLockedState = DoorRef->bIsLocked;
		bHasCapturedOriginal = true;
	}
	else
	{
		UE_LOG(LogLoop9, Warning, TEXT("DoorLock: '%s' is not on a DoorInteractable, so it can never fire"),
			*GetNameSafe(GetOwner()));
	}

	Super::BeginPlay();
}

bool UDoorLockStateAnomalyComponent::ApplyAnomalyState()
{
	if (!DoorRef)
	{
		UE_LOG(LogLoop9, Warning, TEXT("DoorLock: skipped '%s' (no door owner)"), *GetNameSafe(GetOwner()));
		return false;
	}

	if (DoorRef->IsBlockedFromBehind())
	{
		UE_LOG(LogLoop9, Log, TEXT("DoorLock: skipped '%s' (blocked from behind)"),
			*DoorRef->GetActorNameOrLabel());
		return false;
	}

	if (DoorRef->bIsOpen)
	{
		UE_LOG(LogLoop9, Log, TEXT("DoorLock: skipped '%s' (already open)"),
			*DoorRef->GetActorNameOrLabel());
		return false;
	}

	if (!bHasCapturedOriginal)
	{
		bOriginalLockedState = DoorRef->bIsLocked;
		bHasCapturedOriginal = true;
	}

	DoorRef->SetLocked(!DoorRef->bIsLocked);
	UE_LOG(LogLoop9, Log, TEXT("DoorLock: '%s' is now %s"),
		*DoorRef->GetActorNameOrLabel(),
		DoorRef->bIsLocked ? TEXT("LOCKED") : TEXT("unlocked"));
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
