#include "Anomaly/WatcherAnomalyComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/AudioComponent.h"
#include "Controllers/Loop9PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Subsystems/Loop9AchievementsSubsystem.h"
#include "Subsystems/Loop9LightsSubsystem.h"
#include "Subsystems/Loop9ObservationJournalSubsystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float PollIntervalSeconds = 0.1f;
	/** Aim at chest height rather than the feet so a figure behind a desk still counts. */
	constexpr float LookTargetHeight = 120.0f;
	/** Journal subject id; the backend sees "object_inspected" with this slug. */
	const TCHAR* WatcherObservationId = TEXT("figure_back_turned");
}

UWatcherAnomalyComponent::UWatcherAnomalyComponent()
{
	AnomalyProbability = 0.45f;
	AnomalyObjectKind = TEXT("a man standing with his back turned");

	static ConstructorHelpers::FObjectFinder<USoundWave> LookAwayFinder(
		TEXT("/Game/MyStuff/Sound/Watcher/Watcher_LookAway_Piano.Watcher_LookAway_Piano"));
	if (LookAwayFinder.Succeeded())
	{
		LookAwaySound = LookAwayFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> CryingFinder(
		TEXT("/Game/MyStuff/Sound/Watcher/Watcher_Crying.Watcher_Crying"));
	if (CryingFinder.Succeeded())
	{
		CryingSound = CryingFinder.Object;
	}

	// The desk-phone falloff pushed out ~2.5 m with a linear curve: audible across
	// the room, gone by the lift.
	static ConstructorHelpers::FObjectFinder<USoundAttenuation> CryingAttenuationFinder(
		TEXT("/Game/MyStuff/Sound/Watcher/ATT_WatcherCrying.ATT_WatcherCrying"));
	if (CryingAttenuationFinder.Succeeded())
	{
		CryingAttenuation = CryingAttenuationFinder.Object;
	}
}

void UWatcherAnomalyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPolling();
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

	StopPolling();
	DestroyFigure();

	// He faces wherever the anchor faces; the arrow on AWatcherAnchor is the
	// direction of his nose. Nothing is computed here, so what the level
	// designer sees is what spawns.
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

	if (CryingSound && SpawnedFigure->GetRootComponent())
	{
		CryingAudioComponent = UGameplayStatics::SpawnSoundAttached(
			CryingSound, SpawnedFigure->GetRootComponent(), NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset,
			/*bStopWhenAttachedToDestroyed*/ true, 1.0f, 1.0f, 0.0f, CryingAttenuation);
	}

	SpawnedAtSeconds = World->GetTimeSeconds();
	LookStartedAtSeconds = -1.0;
	LastSeenAtSeconds = -1.0;
	LastFigureTarget = SpawnedFigure->GetActorLocation() + FVector(0.0f, 0.0f, LookTargetHeight);
	bEverObserved = false;
	bAwaitingLookBack = false;
	bCausedBlackout = false;

	World->GetTimerManager().SetTimer(
		PollTimerHandle, this, &UWatcherAnomalyComponent::Poll, PollIntervalSeconds, true);

	UE_LOG(LogTemp, Log, TEXT("Watcher anomaly: figure spawned at %s"), *SpawnLocation.ToCompactString());
	return true;
}

void UWatcherAnomalyComponent::RestoreNormalState()
{
	StopPolling();
	DestroyFigure();
	bAwaitingLookBack = false;

	// A reset (next floor, AnomalyReset, run restart) gives the lights back if
	// this component was the one that took them.
	if (bCausedBlackout)
	{
		if (UWorld* World = GetWorld())
		{
			if (ULoop9LightsSubsystem* Lights = World->GetSubsystem<ULoop9LightsSubsystem>())
			{
				Lights->Restore();
			}
		}
		bCausedBlackout = false;
	}
}

void UWatcherAnomalyComponent::Poll()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		StopPolling();
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	// After a quiet vanish: the lights wait for the player to look at where he was.
	if (bAwaitingLookBack)
	{
		if (IsLookingAt(LastFigureTarget, PlayerPawn, PlayerController))
		{
			bAwaitingLookBack = false;
			StopPolling();
			if (LookBackSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this, LookBackSound, LastFigureTarget, FRotator::ZeroRotator,
					1.0f, 1.0f, 0.0f, LookBackAttenuation);
			}
			Blackout();
			UE_LOG(LogTemp, Log, TEXT("Watcher anomaly: player looked back at the empty spot, lights out"));
		}
		return;
	}

	if (!IsValid(SpawnedFigure))
	{
		StopPolling();
		return;
	}

	const double Now = World->GetTimeSeconds();
	if (Now - SpawnedAtSeconds >= MaxLifetimeSeconds)
	{
		UE_LOG(LogTemp, Log, TEXT("Watcher anomaly: figure vanished (lifetime)"));
		StopPolling();
		DestroyFigure();
		return;
	}

	LastFigureTarget = SpawnedFigure->GetActorLocation() + FVector(0.0f, 0.0f, LookTargetHeight);

	const float Distance = FVector::Dist(PlayerPawn->GetActorLocation(), SpawnedFigure->GetActorLocation());
	if (Distance <= VanishDistance)
	{
		VanishWithBurst(TEXT("player too close"), true);
		return;
	}

	const bool bObserved = IsLookingAt(LastFigureTarget, PlayerPawn, PlayerController);
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
			LookStartedAtSeconds = Now;
		}
		LastSeenAtSeconds = Now;

		if (MaxContinuousLookSeconds > 0.0f && Now - LookStartedAtSeconds >= MaxContinuousLookSeconds)
		{
			VanishWithBurst(TEXT("stared too long"), false);
			return;
		}
	}
	else if (LookStartedAtSeconds >= 0.0 && Now - LastSeenAtSeconds >= MinLookAwaySeconds)
	{
		// A real look-away, not a doorframe crossing the trace: he is gone the
		// moment the player is not looking. The lights wait for the look back.
		VanishQuietly(TEXT("looked away"));
	}
}

bool UWatcherAnomalyComponent::IsLookingAt(const FVector& Target, const APawn* PlayerPawn, const APlayerController* PlayerController) const
{
	if (!PlayerPawn || !PlayerController || !PlayerController->PlayerCameraManager)
	{
		return false;
	}

	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = PlayerController->PlayerCameraManager->GetActorForwardVector().GetSafeNormal();
	const FVector ToTarget = (Target - CameraLocation).GetSafeNormal();

	if (FVector::DotProduct(CameraForward, ToTarget) < LookDotThreshold)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WatcherObservedTrace), true, PlayerPawn);
	const bool bHitSomething = GetWorld()->LineTraceSingleByChannel(
		Hit, CameraLocation, Target, ECC_Visibility, Params);

	// Nothing in the way, or the only thing in the way is him.
	return !bHitSomething || (IsValid(SpawnedFigure) && Hit.GetActor() == SpawnedFigure);
}

void UWatcherAnomalyComponent::VanishWithBurst(const TCHAR* Reason, bool bApproached)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	StopPolling();

	// Burst first so the frame he disappears on is already unreadable.
	if (ALoop9PlayerController* PC = Cast<ALoop9PlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
	{
		PC->PlaySignalBurst(BurstSeconds);
	}

	Blackout();

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

void UWatcherAnomalyComponent::VanishQuietly(const TCHAR* Reason)
{
	if (IsValid(SpawnedFigure))
	{
		if (LookAwaySound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this, LookAwaySound, SpawnedFigure->GetActorLocation(), FRotator::ZeroRotator,
				1.0f, 1.0f, 0.0f, LookAwayAttenuation);
		}
		else if (VanishSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this, VanishSound, SpawnedFigure->GetActorLocation(), FRotator::ZeroRotator,
				1.0f, 1.0f, 0.0f, VanishAttenuation);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Watcher anomaly: figure vanished (%s), waiting for the look back"), Reason);
	DestroyFigure();
	// Polling continues: the next look at the empty spot cuts the lights.
	bAwaitingLookBack = bBlackout;
	if (!bAwaitingLookBack)
	{
		StopPolling();
	}
}

void UWatcherAnomalyComponent::Blackout()
{
	if (!bBlackout)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (ULoop9LightsSubsystem* Lights = World->GetSubsystem<ULoop9LightsSubsystem>())
		{
			// 0 = the floor stays dark until the next loop or a reset restores it.
			Lights->BlackoutForSeconds(BlackoutSeconds, TArray<ULightComponent*>(), /*bPlayWatcherAudio*/ true);
			bCausedBlackout = true;
		}
	}
}

void UWatcherAnomalyComponent::StopPolling()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}
}

void UWatcherAnomalyComponent::DestroyFigure()
{
	if (IsValid(CryingAudioComponent))
	{
		CryingAudioComponent->Stop();
	}
	CryingAudioComponent = nullptr;

	if (IsValid(SpawnedFigure))
	{
		SpawnedFigure->Destroy();
	}
	SpawnedFigure = nullptr;
}
