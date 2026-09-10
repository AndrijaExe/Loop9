#include "Anomaly/WatcherAnomalyComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Controllers/Loop9PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Subsystems/Loop9AchievementsSubsystem.h"
#include "Subsystems/Loop9LightsSubsystem.h"
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

	// He faces wherever the anchor faces; the arrow on AWatcherAnchor is the
	// direction of his back-of-head-to-nose line. Nothing is computed here, so
	// what the level designer sees is what spawns.
	const FVector SpawnLocation = Owner->GetActorLocation();
	const FRotator SpawnRotation = Owner->GetActorRotation();

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
	LastSeenAtSeconds = -1.0;
	bEverObserved = false;
	bLookedAway = false;

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
		VanishForPlayer(TEXT("player too close"), true);
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

		if (LookStartedAtSeconds < 0.0)
		{
			// First sight, or sight resumed after a real look-away: he is not
			// there anymore. A gap shorter than MinLookAwaySeconds never gets
			// here, so a doorframe crossing the trace does not count as a look.
			if (bLookedAway)
			{
				VanishForPlayer(TEXT("second look"), false);
				return;
			}
			LookStartedAtSeconds = Now;
		}
		LastSeenAtSeconds = Now;

		if (MaxContinuousLookSeconds > 0.0f && Now - LookStartedAtSeconds >= MaxContinuousLookSeconds)
		{
			VanishForPlayer(TEXT("stared too long"), false);
			return;
		}
	}
	else if (LookStartedAtSeconds >= 0.0 && Now - LastSeenAtSeconds >= MinLookAwaySeconds)
	{
		bLookedAway = true;
		LookStartedAtSeconds = -1.0;
	}
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

void UWatcherAnomalyComponent::VanishForPlayer(const TCHAR* Reason, bool bApproached)
{
	UWorld* World = GetWorld();
	if (!World || !bEffectOnPlayerCausedVanish)
	{
		Vanish(Reason);
		return;
	}

	// Burst first so the frame he disappears on is already unreadable.
	if (ALoop9PlayerController* PC = Cast<ALoop9PlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
	{
		PC->PlaySignalBurst(BurstSeconds);
	}

	if (bBlackout)
	{
		if (ULoop9LightsSubsystem* Lights = World->GetSubsystem<ULoop9LightsSubsystem>())
		{
			// 0 = the floor stays dark until the next loop restores it.
			Lights->BlackoutForSeconds(BlackoutSeconds, TArray<ULightComponent*>());
		}
	}

	if (bApproached)
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (ULoop9AchievementsSubsystem* Achievements = GameInstance->GetSubsystem<ULoop9AchievementsSubsystem>())
			{
				Achievements->UnlockAchievement(FName(TEXT("ACH_TOO_CLOSE")));
			}
		}
	}

	Vanish(Reason);
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
