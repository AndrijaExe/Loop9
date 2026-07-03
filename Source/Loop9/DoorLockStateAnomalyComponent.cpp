#include "DoorLockStateAnomalyComponent.h"
#include "Subsystems/AnomalyManager.h"
#include "DoorInteractable.h"
#include "Kismet/GameplayStatics.h"

UDoorLockStateAnomalyComponent::UDoorLockStateAnomalyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDoorLockStateAnomalyComponent::BeginPlay()
{
	Super::BeginPlay();

	DoorRef = Cast<ADoorInteractable>(GetOwner());
	if (DoorRef)
	{
		bOriginalLockedState = DoorRef->bIsLocked;
		bHasCapturedOriginal = true;
	}

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		if (UAnomalyManager* Manager = GI->GetSubsystem<UAnomalyManager>())
		{
			Manager->RegisterAnomaly(GetOwner());
		}
	}
}

void UDoorLockStateAnomalyComponent::ActivateAnomaly()
{
	if (bIsAnomalyActive || !DoorRef)
	{
		return;
	}

	if (!bHasCapturedOriginal)
	{
		bOriginalLockedState = DoorRef->bIsLocked;
		bHasCapturedOriginal = true;
	}

	bIsAnomalyActive = true;
	DoorRef->SetLocked(!DoorRef->bIsLocked);
}

void UDoorLockStateAnomalyComponent::DeactivateAnomaly()
{
	if (!bIsAnomalyActive || !DoorRef)
	{
		return;
	}

	bIsAnomalyActive = false;
	if (bHasCapturedOriginal)
	{
		DoorRef->SetLocked(bOriginalLockedState);
	}
}
