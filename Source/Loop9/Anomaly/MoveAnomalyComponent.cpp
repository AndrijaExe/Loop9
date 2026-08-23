#include "Anomaly/MoveAnomalyComponent.h"

#include "Anomaly/AnomalyMovePoint.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Loop9.h"

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

TArray<AAnomalyMovePoint*> UMoveAnomalyComponent::GatherEligiblePoints() const
{
	TArray<AAnomalyMovePoint*> Points;

	for (const TObjectPtr<AAnomalyMovePoint>& Point : MoveTargetPoints)
	{
		if (IsValid(Point) && Point->bEnabled)
		{
			Points.Add(Point);
		}
	}

	if (Points.Num() > 0)
	{
		return Points;
	}

	// The level-wide search is opt-in through the tag. Without it, dropping a move
	// point anywhere in the map would silently retarget every other move anomaly
	// on the floor, including ones whose destination was authored years earlier.
	if (MoveTargetTag.IsNone())
	{
		return Points;
	}

	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<AAnomalyMovePoint> It(World); It; ++It)
		{
			AAnomalyMovePoint* Point = *It;
			if (IsValid(Point) && Point->bEnabled && Point->PointTag == MoveTargetTag)
			{
				Points.Add(Point);
			}
		}
	}

	return Points;
}

AAnomalyMovePoint* UMoveAnomalyComponent::DrawTargetPoint(const TArray<AAnomalyMovePoint*>& Points) const
{
	if (Points.IsEmpty())
	{
		return nullptr;
	}

	TArray<AAnomalyMovePoint*> Eligible = Points;
	if (bAvoidRepeatingLastPoint && Eligible.Num() > 1 && LastUsedPoint.IsValid())
	{
		Eligible.Remove(LastUsedPoint.Get());
	}

	if (Eligible.IsEmpty())
	{
		Eligible = Points;
	}

	float TotalWeight = 0.0f;
	for (const AAnomalyMovePoint* Point : Eligible)
	{
		TotalWeight += FMath::Max(Point->Weight, 0.0f);
	}

	if (TotalWeight <= 0.0f)
	{
		return Eligible[FMath::RandRange(0, Eligible.Num() - 1)];
	}

	float Target = FMath::FRandRange(0.0f, TotalWeight);
	for (AAnomalyMovePoint* Point : Eligible)
	{
		Target -= FMath::Max(Point->Weight, 0.0f);
		if (Target <= 0.0f)
		{
			return Point;
		}
	}

	return Eligible.Last();
}

bool UMoveAnomalyComponent::ApplyAnomalyState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	const FString OwnerName = Owner->GetActorNameOrLabel();

	FVector TargetLocation;
	FRotator TargetRotation;

	if (AAnomalyMovePoint* Point = DrawTargetPoint(GatherEligiblePoints()))
	{
		TargetLocation = Point->GetActorLocation();
		TargetRotation = Point->GetActorRotation();
		LastUsedPoint = Point;
	}
	else if (!AnomalyLocation.IsNearlyZero())
	{
		TargetLocation = bAnomalyTransformIsWorldSpace
			? AnomalyLocation
			: Owner->GetActorTransform().TransformPosition(AnomalyLocation);

		TargetRotation = bAnomalyTransformIsWorldSpace
			? AnomalyRotation
			: (Owner->GetActorRotation() + AnomalyRotation);
	}
	else
	{
		// Refusing beats guessing. The manager simply draws another anomaly, so
		// the floor still has something to find, while an unauthored move would
		// drop the object at the world origin where the player cannot follow.
		UE_LOG(LogLoop9, Warning,
			TEXT("MoveAnomaly on '%s': no move points found (tag='%s') and AnomalyLocation is unset, skipping."),
			*OwnerName,
			*MoveTargetTag.ToString());
		return false;
	}

	// A destination the player cannot tell apart from the original fails the same
	// way an unchanged material does: the floor insists something moved and the
	// player is punished for correctly seeing that nothing did.
	if (TargetLocation.Equals(NormalLocation, 20.0f))
	{
		UE_LOG(LogLoop9, Warning,
			TEXT("MoveAnomaly on '%s': destination sits on the object's normal position, skipping."),
			*OwnerName);
		return false;
	}

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
