#include "Anomaly/Pursuer/PursuerAnomalyComponent.h"

#include "Anomaly/Pursuer/PursuerAnomalyCharacter.h"
#include "Anomaly/Pursuer/PursuerSpawnPoint.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

UPursuerAnomalyComponent::UPursuerAnomalyComponent()
{
	AnomalyProbability = 0.4f;
}

bool UPursuerAnomalyComponent::ApplyAnomalyState()
{
	if (!PursuerClass)
	{
		return false;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn)
	{
		return false;
	}

	FTransform SpawnTransform;
	bool bFoundSpawn = false;

	if (bUseSpawnPoints)
	{
		bFoundSpawn = TryGetSpawnTransformFromPoints(PlayerPawn, SpawnTransform);
	}

	if (!bFoundSpawn && bFallbackToRadiusSpawn)
	{
		bFoundSpawn = TryGetSpawnTransformFromRadius(PlayerPawn, SpawnTransform);
	}

	if (!bFoundSpawn)
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedPursuer = GetWorld()->SpawnActor<APursuerAnomalyCharacter>(PursuerClass, SpawnTransform.GetLocation(), SpawnTransform.Rotator(), Params);
	if (!SpawnedPursuer)
	{
		return false;
	}

	SpawnedPursuer->OnDestroyed.AddDynamic(this, &UPursuerAnomalyComponent::OnSpawnedPursuerDestroyed);

	if (ActiveAnomalyLoopSound)
	{
		ActiveAnomalyLoopAudioComponent = UGameplayStatics::SpawnSoundAttached(
			ActiveAnomalyLoopSound,
			SpawnedPursuer->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset,
			true,
			ActiveAnomalyLoopVolume,
			1.0f,
			0.0f,
			ActiveAnomalyLoopAttenuation,
			nullptr,
			true);

		if (ActiveAnomalyLoopAudioComponent)
		{
			ActiveAnomalyLoopAudioComponent->bAutoDestroy = false;
		}
	}

	return true;
}

void UPursuerAnomalyComponent::RestoreNormalState()
{
	if (ActiveAnomalyLoopAudioComponent)
	{
		ActiveAnomalyLoopAudioComponent->Stop();
		ActiveAnomalyLoopAudioComponent->DestroyComponent();
		ActiveAnomalyLoopAudioComponent = nullptr;
	}

	if (SpawnedPursuer)
	{
		SpawnedPursuer->Destroy();
		SpawnedPursuer = nullptr;
	}
}

void UPursuerAnomalyComponent::OnSpawnedPursuerDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == SpawnedPursuer)
	{
		if (ActiveAnomalyLoopAudioComponent)
		{
			ActiveAnomalyLoopAudioComponent->Stop();
			ActiveAnomalyLoopAudioComponent->DestroyComponent();
			ActiveAnomalyLoopAudioComponent = nullptr;
		}

		SpawnedPursuer = nullptr;
		bIsAnomalyActive = false;
	}
}

bool UPursuerAnomalyComponent::TryGetSpawnTransformFromPoints(APawn* PlayerPawn, FTransform& OutTransform) const
{
	if (!GetWorld() || !PlayerPawn)
	{
		return false;
	}

	TArray<APursuerSpawnPoint*> Candidates;
	for (APursuerSpawnPoint* P : ManualSpawnPoints)
	{
		if (IsValid(P) && P->bEnabled)
		{
			Candidates.Add(P);
		}
	}

	if (Candidates.Num() == 0)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APursuerSpawnPoint::StaticClass(), FoundActors);
		for (AActor* A : FoundActors)
		{
			APursuerSpawnPoint* Point = Cast<APursuerSpawnPoint>(A);
			if (!Point || !Point->bEnabled)
			{
				continue;
			}

			if (!SpawnPointTag.IsNone() && !Point->ActorHasTag(SpawnPointTag))
			{
				continue;
			}

			Candidates.Add(Point);
		}
	}

	if (Candidates.Num() == 0)
	{
		return false;
	}

	const int32 Attempts = FMath::Min(SpawnSearchMaxAttempts, Candidates.Num());
	for (int32 i = 0; i < Attempts; ++i)
	{
		const int32 Index = FMath::RandRange(0, Candidates.Num() - 1);
		APursuerSpawnPoint* Chosen = Candidates[Index];
		Candidates.RemoveAtSwap(Index);

		if (!IsValid(Chosen))
		{
			continue;
		}

		FVector NavLocation;
		if (ProjectToNavigation(Chosen->GetActorLocation() + FVector(0, 0, SpawnHeightOffset), NavLocation))
		{
			const FRotator SpawnRotation = (PlayerPawn->GetActorLocation() - NavLocation).Rotation();
			OutTransform = FTransform(SpawnRotation, NavLocation);
			return true;
		}
	}

	return false;
}

bool UPursuerAnomalyComponent::TryGetSpawnTransformFromRadius(APawn* PlayerPawn, FTransform& OutTransform) const
{
	if (!GetWorld() || !PlayerPawn)
	{
		return false;
	}

	for (int32 i = 0; i < SpawnSearchMaxAttempts; ++i)
	{
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float Radius = FMath::FRandRange(FMath::Min(SpawnRadiusMin, SpawnRadiusMax), FMath::Max(SpawnRadiusMin, SpawnRadiusMax));
		const FVector SpawnOffset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, SpawnHeightOffset);
		const FVector RawSpawnLocation = PlayerPawn->GetActorLocation() + SpawnOffset;

		FVector NavLocation;
		if (ProjectToNavigation(RawSpawnLocation, NavLocation))
		{
			const FRotator SpawnRotation = (PlayerPawn->GetActorLocation() - NavLocation).Rotation();
			OutTransform = FTransform(SpawnRotation, NavLocation);
			return true;
		}
	}

	return false;
}

bool UPursuerAnomalyComponent::ProjectToNavigation(const FVector& RawLocation, FVector& OutNavLocation) const
{
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation NavLoc;
		const FVector Extent(NavProjectionExtent, NavProjectionExtent, NavProjectionExtent);
		if (NavSys->ProjectPointToNavigation(RawLocation, NavLoc, Extent))
		{
			OutNavLocation = NavLoc.Location;
			return true;
		}
	}

	return false;
}
