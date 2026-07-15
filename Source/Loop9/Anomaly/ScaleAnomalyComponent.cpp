#include "Anomaly/ScaleAnomalyComponent.h"

#include "Components/SceneComponent.h"

UScaleAnomalyComponent::UScaleAnomalyComponent()
{
	AnomalyProbability = 0.5f;
}

void UScaleAnomalyComponent::BeginPlay()
{
	if (AActor* Owner = GetOwner())
	{
		if (bEnsureOwnerIsMovable)
		{
			if (USceneComponent* RootComp = Owner->GetRootComponent())
			{
				if (RootComp->Mobility != EComponentMobility::Movable)
				{
					RootComp->SetMobility(EComponentMobility::Movable);
				}
			}
		}

		if (bCaptureNormalScaleOnBeginPlay)
		{
			NormalScale = Owner->GetActorScale3D();
		}
	}

	Super::BeginPlay();
}

bool UScaleAnomalyComponent::ApplyAnomalyState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	Owner->SetActorScale3D(NormalScale * ScaleMultiplier);
	return true;
}

void UScaleAnomalyComponent::RestoreNormalState()
{
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorScale3D(NormalScale);
	}
}
