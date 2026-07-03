#include "Anomaly/AnomalyComponentBase.h"

#include "Subsystems/AnomalyManager.h"
#include "Kismet/GameplayStatics.h"

UAnomalyComponentBase::UAnomalyComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAnomalyComponentBase::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		if (UAnomalyManager* AnomalyManager = GameInstance->GetSubsystem<UAnomalyManager>())
		{
			AnomalyManager->RegisterAnomalyComponent(this);
		}
	}
}

void UAnomalyComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		if (UAnomalyManager* AnomalyManager = GameInstance->GetSubsystem<UAnomalyManager>())
		{
			AnomalyManager->UnregisterAnomalyComponent(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UAnomalyComponentBase::ActivateAnomaly()
{
	if (bIsAnomalyActive)
	{
		return;
	}

	if (!ApplyAnomalyState())
	{
		return;
	}

	bIsAnomalyActive = true;
	OnAnomalyActivated.Broadcast(true);
}

void UAnomalyComponentBase::DeactivateAnomaly()
{
	if (!bIsAnomalyActive)
	{
		return;
	}

	bIsAnomalyActive = false;
	RestoreNormalState();
	OnAnomalyDeactivated.Broadcast(false);
}

void UAnomalyComponentBase::ResetToNormalState()
{
	RestoreNormalState();
}

bool UAnomalyComponentBase::ApplyAnomalyState()
{
	return true;
}

void UAnomalyComponentBase::RestoreNormalState()
{
}
