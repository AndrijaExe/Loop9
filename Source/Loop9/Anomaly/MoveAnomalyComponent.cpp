#include "Anomaly/MoveAnomalyComponent.h"

#include "Components/SceneComponent.h"

UMoveAnomalyComponent::UMoveAnomalyComponent()
{
	AnomalyProbability = 0.5f;
}

void UMoveAnomalyComponent::BeginPlay()
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

		if (bCaptureNormalTransformOnBeginPlay)
		{
			NormalLocation = Owner->GetActorLocation();
			NormalRotation = Owner->GetActorRotation();
		}
	}

	Super::BeginPlay();
}

bool UMoveAnomalyComponent::ApplyAnomalyState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	const FVector TargetLocation = bAnomalyTransformIsWorldSpace
		? AnomalyLocation
		: Owner->GetActorTransform().TransformPosition(AnomalyLocation);

	const FRotator TargetRotation = bAnomalyTransformIsWorldSpace
		? AnomalyRotation
		: (Owner->GetActorRotation() + AnomalyRotation);

	if (bUseTeleport)
	{
		Owner->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		Owner->SetActorLocationAndRotation(TargetLocation, TargetRotation);
	}

	return true;
}

void UMoveAnomalyComponent::RestoreNormalState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (bUseTeleport)
	{
		Owner->SetActorLocationAndRotation(NormalLocation, NormalRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		Owner->SetActorLocationAndRotation(NormalLocation, NormalRotation);
	}
}
