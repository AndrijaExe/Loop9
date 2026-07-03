#include "MoveAnomalyComponent.h"
#include "Subsystems/AnomalyManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"

UMoveAnomalyComponent::UMoveAnomalyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMoveAnomalyComponent::BeginPlay()
{
	Super::BeginPlay();

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

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		if (UAnomalyManager* Manager = GI->GetSubsystem<UAnomalyManager>())
		{
			Manager->RegisterAnomaly(GetOwner());
		}
	}
}

void UMoveAnomalyComponent::ActivateAnomaly()
{
	if (bIsAnomalyActive)
	{
		return;
	}

	bIsAnomalyActive = true;
	if (AActor* Owner = GetOwner())
	{
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
	}
}

void UMoveAnomalyComponent::DeactivateAnomaly()
{
	if (!bIsAnomalyActive)
	{
		return;
	}

	bIsAnomalyActive = false;
	if (AActor* Owner = GetOwner())
	{
		if (bUseTeleport)
		{
			Owner->SetActorLocationAndRotation(NormalLocation, NormalRotation, false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			Owner->SetActorLocationAndRotation(NormalLocation, NormalRotation);
		}
	}
}
