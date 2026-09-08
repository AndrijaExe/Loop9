#include "Anomaly/WatcherAnomalyComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Subsystems/Loop9ObservationJournalSubsystem.h"
#include "TimerManager.h"

namespace
{
	constexpr float PollIntervalSeconds = 0.1f;
	/** Journal subject id; the backend sees "object_inspected" with this slug. */
	const TCHAR* WatcherObservationId = TEXT("figure_back_turned");
}

UWatcherAnomalyComponent::UWatcherAnomalyComponent()
{
	AnomalyProbability = 0.45f;
	AnomalyObjectKind = TEXT("a man standing with his back turned");
}

void UWatcherAnomalyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyFigure();
	Super::EndPlay(EndPlayReason);
}

bool UWatcherAnomalyComponent::ApplyAnomalyState()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner || !FigureClass)
	{
		return false;
	}

	DestroyFigure();

	FVector SpawnLocation = Owner->GetActorLocation();
	FRotator SpawnRotation = Owner->GetActorRotation();

	// Back to the player: face the direction the player would look along.
	if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		const FVector Away = SpawnLocation - PlayerPawn->GetActorLocation();
		if (!Away.IsNearlyZero())
		{
			SpawnRotation = FRotator(0.0f, Away.Rotation().Yaw, 0.0f);
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = Owner;
	SpawnedFigure = World->SpawnActor<AActor>(FigureClass, SpawnLocation, SpawnRotation, Params);
	if (!SpawnedFigure)
	{
		return false;
	}

	SpawnedAtSeconds = World->GetTimeSeconds();
	LookStartedAtSeconds = -1.0;
	bEverObserved = false;
	bLookedAway = false;
	bWasObservedLastPoll = false;

	World->GetTimerManager().SetTimer(
		PollTimerHandle, this, &UWatcherAnomalyComponent::Poll, PollIntervalSeconds, true);

	UE_LOG(LogTemp, Log, TEXT("Watcher anomaly: figure spawned at %s"), *SpawnLocation.ToCompactString());
	return true;
}

void UWatcherAnomalyComponent::RestoreNormalState()
{
	DestroyFigure();
}

void UWatcherAnomalyComponent::Poll()
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(SpawnedFigure))
	{
		DestroyFigure();
		return;
	}

	const double Now = World->GetTimeSeconds();
	if (Now - SpawnedAtSeconds >= MaxLifetimeSeconds)
	{
		Vanish(TEXT("lifetime"));
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	const float Distance = FVector::Dist(PlayerPawn->GetActorLocation(), SpawnedFigure->GetActorLocation());
	if (Distance <= VanishDistance)
	{
		Vanish(TEXT("player too close"));
		return;
	}

	const bool bObserved = IsObservedByPlayer(PlayerPawn, PlayerController);
	if (bObserved)
	{
		if (!bEverObserved)
		{
			bEverObserved = true;
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (ULoop9ObservationJournalSubsystem* Journal =
					GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
				{
					Journal->RecordObjectInspected(FName(WatcherObservationId));
				}
			}
		}

		if (!bWasObservedLastPoll)
		{
			LookStartedAtSeconds = Now;
			// Looked away once and now looking back: he is not there anymore.
			if (bLookedAway)
			{
				Vanish(TEXT("second look"));
				return;
			}
		}
		else if (MaxContinuousLookSeconds > 0.0f
			&& LookStartedAtSeconds >= 0.0
			&& Now - LookStartedAtSeconds >= MaxContinuousLookSeconds)
		{
			Vanish(TEXT("stared too long"));
			return;
		}
	}
	else if (bWasObservedLastPoll)
	{
		bLookedAway = true;
		LookStartedAtSeconds = -1.0;
	}

	bWasObservedLastPoll = bObserved;
}

bool UWatcherAnomalyComponent::IsObservedByPlayer(const APawn* PlayerPawn, const APlayerController* PlayerController) const
{
	if (!PlayerPawn || !PlayerController || !PlayerController->PlayerCameraManager || !IsValid(SpawnedFigure))
	{
		return false;
	}

	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = PlayerController->PlayerCameraManager->GetActorForwardVector().GetSafeNormal();
	// Aim at chest height rather than the feet so a figure behind a desk still counts.
	const FVector Target = SpawnedFigure->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f);
	const FVector ToFigure = (Target - CameraLocation).GetSafeNormal();

	if (FVector::DotProduct(CameraForward, ToFigure) < LookDotThreshold)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WatcherObservedTrace), true, PlayerPawn);
	const bool bHitSomething = GetWorld()->LineTraceSingleByChannel(
		Hit, CameraLocation, Target, ECC_Visibility, Params);

	return !bHitSomething || Hit.GetActor() == SpawnedFigure;
}

void UWatcherAnomalyComponent::Vanish(const TCHAR* Reason)
{
	if (IsValid(SpawnedFigure) && VanishSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, VanishSound, SpawnedFigure->GetActorLocation(), FRotator::ZeroRotator,
			1.0f, 1.0f, 0.0f, VanishAttenuation);
	}
	UE_LOG(LogTemp, Log, TEXT("Watcher anomaly: figure vanished (%s)"), Reason);
	// The floor stays "wrong" — bIsAnomalyActive is untouched, only the manifestation leaves.
	DestroyFigure();
}

void UWatcherAnomalyComponent::DestroyFigure()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}
	if (IsValid(SpawnedFigure))
	{
		SpawnedFigure->Destroy();
	}
	SpawnedFigure = nullptr;
}
