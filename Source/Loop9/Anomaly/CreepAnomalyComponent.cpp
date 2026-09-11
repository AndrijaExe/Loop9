#include "Anomaly/CreepAnomalyComponent.h"

#include "Anomaly/AnomalyMovePoint.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Loop9.h"

UCreepAnomalyComponent::UCreepAnomalyComponent()
{
	AnomalyProbability = 0.5f;
	AnomalyObjectKind = TEXT("an object that is slowly moving on its own");
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCreepAnomalyComponent::BeginPlay()
{
	if (AActor* Owner = GetOwner())
	{
		if (bEnsureOwnerIsMovable)
		{
			// Every scene component, not just the root: a Static mesh under a
			// Movable root stays where it was and only logs a warning.
			TArray<USceneComponent*> SceneComponents;
			Owner->GetComponents<USceneComponent>(SceneComponents);
			for (USceneComponent* SceneComponent : SceneComponents)
			{
				if (SceneComponent && SceneComponent->Mobility != EComponentMobility::Movable)
				{
					SceneComponent->SetMobility(EComponentMobility::Movable);
				}
			}
		}

		NormalLocation = Owner->GetActorLocation();
		NormalRotation = Owner->GetActorRotation();
	}

	Super::BeginPlay();
}

float UCreepAnomalyComponent::GetDriftedDistance() const
{
	const AActor* Owner = GetOwner();
	return Owner ? static_cast<float>(FVector::Dist(Owner->GetActorLocation(), NormalLocation)) : 0.0f;
}

AAnomalyMovePoint* UCreepAnomalyComponent::DrawTargetPoint() const
{
	TArray<AAnomalyMovePoint*> Points;
	for (const TObjectPtr<AAnomalyMovePoint>& Point : CreepTargetPoints)
	{
		if (IsValid(Point) && Point->bEnabled)
		{
			Points.Add(Point);
		}
	}

	if (Points.IsEmpty() && !CreepTargetTag.IsNone())
	{
		if (const UWorld* World = GetWorld())
		{
			for (TActorIterator<AAnomalyMovePoint> It(World); It; ++It)
			{
				AAnomalyMovePoint* Point = *It;
				if (IsValid(Point) && Point->bEnabled && Point->PointTag == CreepTargetTag)
				{
					Points.Add(Point);
				}
			}
		}
	}

	if (Points.IsEmpty())
	{
		return nullptr;
	}

	float TotalWeight = 0.0f;
	for (const AAnomalyMovePoint* Point : Points)
	{
		TotalWeight += FMath::Max(Point->Weight, 0.0f);
	}
	if (TotalWeight <= 0.0f)
	{
		return Points[FMath::RandRange(0, Points.Num() - 1)];
	}

	float Target = FMath::FRandRange(0.0f, TotalWeight);
	for (AAnomalyMovePoint* Point : Points)
	{
		Target -= FMath::Max(Point->Weight, 0.0f);
		if (Target <= 0.0f)
		{
			return Point;
		}
	}
	return Points.Last();
}

bool UCreepAnomalyComponent::ApplyAnomalyState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// Always start from the normal spot: the trick is that nothing is wrong
	// when the lift opens.
	Owner->SetActorLocationAndRotation(NormalLocation, NormalRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (const AAnomalyMovePoint* Point = DrawTargetPoint())
	{
		TargetLocation = Point->GetActorLocation();
		TargetRotation = Point->GetActorRotation();
	}
	else
	{
		TargetLocation = NormalLocation + NormalRotation.RotateVector(CreepOffset);
		TargetRotation = NormalRotation + CreepRotation;
	}

	TotalDistance = static_cast<float>(FVector::Dist(NormalLocation, TargetLocation));
	if (TotalDistance < 5.0f)
	{
		// A drift the player could never notice fails the same way a Move onto
		// its own spot does: the floor insists, the player is punished.
		UE_LOG(LogLoop9, Warning,
			TEXT("CreepAnomaly on '%s': destination is %.1f cm from the normal spot, skipping."),
			*Owner->GetActorNameOrLabel(), TotalDistance);
		return false;
	}

	bDrifting = true;
	SetComponentTickEnabled(true);
	UE_LOG(LogLoop9, Log, TEXT("CreepAnomaly on '%s': drifting %.0f cm at %.2f cm/s"),
		*Owner->GetActorNameOrLabel(), TotalDistance, CreepSpeedCmPerSecond);
	return true;
}

void UCreepAnomalyComponent::RestoreNormalState()
{
	bDrifting = false;
	SetComponentTickEnabled(false);
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocationAndRotation(NormalLocation, NormalRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UCreepAnomalyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!bDrifting || !Owner)
	{
		return;
	}

	if (bPauseWhileObserved && IsObservedByPlayer())
	{
		return;
	}

	const FVector Current = Owner->GetActorLocation();
	const float Remaining = static_cast<float>(FVector::Dist(Current, TargetLocation));
	const float Step = CreepSpeedCmPerSecond * DeltaTime;

	if (Remaining <= Step)
	{
		Owner->SetActorLocationAndRotation(TargetLocation, TargetRotation);
		bDrifting = false;
		SetComponentTickEnabled(false);
		return;
	}

	const FVector Next = Current + (TargetLocation - Current).GetSafeNormal() * Step;
	// Rotation follows the same fraction of the path so both finish together.
	const float Alpha = TotalDistance > 0.0f
		? FMath::Clamp(1.0f - (Remaining - Step) / TotalDistance, 0.0f, 1.0f)
		: 1.0f;
	const FRotator NextRotation = FMath::Lerp(NormalRotation, TargetRotation, Alpha);
	Owner->SetActorLocationAndRotation(Next, NextRotation);
}

bool UCreepAnomalyComponent::IsObservedByPlayer() const
{
	const AActor* Owner = GetOwner();
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!Owner || !PlayerController || !PlayerController->PlayerCameraManager)
	{
		return false;
	}

	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = PlayerController->PlayerCameraManager->GetActorForwardVector().GetSafeNormal();
	const FVector ToObject = (Owner->GetActorLocation() - CameraLocation).GetSafeNormal();
	return FVector::DotProduct(CameraForward, ToObject) >= LookDotThreshold;
}
