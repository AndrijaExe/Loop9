#include "Anomaly/Pursuer/PursuerAnomalyComponent.h"

#include "Anomaly/Pursuer/PursuerAnomalyCharacter.h"
#include "Anomaly/Pursuer/PursuerSpawnPoint.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Loop9.h"
#include "NavigationSystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UPursuerAnomalyComponent::UPursuerAnomalyComponent()
{
	AnomalyProbability = 0.4f;

	static ConstructorHelpers::FObjectFinder<USoundBase> TensionFinder(
		TEXT("/Game/MyStuff/Sound/Pursuer/185993__mmleys__dynamic-space"));
	if (TensionFinder.Succeeded())
	{
		ActiveAnomalyLoopSound = TensionFinder.Object;
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<USoundBase> TensionFallbackFinder(
			TEXT("/Game/MyStuff/Sound/Pursuer/PursuerTensionLoop"));
		if (TensionFallbackFinder.Succeeded())
		{
			ActiveAnomalyLoopSound = TensionFallbackFinder.Object;
		}
		else
		{
			static ConstructorHelpers::FObjectFinder<USoundBase> MenuAmbientFinder(
				TEXT("/Game/MyStuff/Sound/MainMenu/HorrorAmbientSound"));
			if (MenuAmbientFinder.Succeeded())
			{
				ActiveAnomalyLoopSound = MenuAmbientFinder.Object;
			}
		}
	}
}

void UPursuerAnomalyComponent::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	TArray<APursuerSpawnPoint*> Points;
	CollectEnabledSpawnPoints(Points);
	UE_LOG(LogLoop9, Log, TEXT("Pursuer on '%s': %d spawn point(s), fallback radius=%s"),
		GetOwner() ? *GetOwner()->GetActorNameOrLabel() : TEXT("None"),
		Points.Num(),
		bFallbackToRadiusSpawn ? TEXT("on") : TEXT("off"));
	for (const APursuerSpawnPoint* Point : Points)
	{
		if (Point)
		{
			UE_LOG(LogLoop9, Log, TEXT("  spawn point '%s' at %s"),
				*Point->GetActorNameOrLabel(), *Point->GetActorLocation().ToCompactString());
		}
	}
#endif
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
	bool bFromAuthoredPoint = false;

	if (bUseSpawnPoints)
	{
		bFoundSpawn = TryGetSpawnTransformFromPoints(PlayerPawn, SpawnTransform);
		bFromAuthoredPoint = bFoundSpawn;
	}

	if (!bFoundSpawn && bFallbackToRadiusSpawn)
	{
		bFoundSpawn = TryGetSpawnTransformFromRadius(PlayerPawn, SpawnTransform);
	}

	if (!bFoundSpawn)
	{
		UE_LOG(LogLoop9, Warning, TEXT("Pursuer: no spawn transform (points and radius both failed)"));
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedPursuer = GetWorld()->SpawnActor<APursuerAnomalyCharacter>(PursuerClass, SpawnTransform.GetLocation(), SpawnTransform.Rotator(), Params);
	if (!SpawnedPursuer)
	{
		return false;
	}

	UE_LOG(LogLoop9, Log, TEXT("Pursuer spawned via %s at %s"),
		bFromAuthoredPoint ? TEXT("authored spawn point") : TEXT("radius fallback around player"),
		*SpawnTransform.GetLocation().ToCompactString());

	SpawnedPursuer->OnDestroyed.AddDynamic(this, &UPursuerAnomalyComponent::OnSpawnedPursuerDestroyed);
	if (TensionMusicDelaySeconds <= 0.0f)
	{
		StartTensionMusic();
	}
	else if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TensionMusicDelayHandle,
			this,
			&UPursuerAnomalyComponent::StartTensionMusic,
			TensionMusicDelaySeconds,
			false);
	}

	return true;
}

void UPursuerAnomalyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The base class does not restore anomaly state on EndPlay, so a pursuer torn
	// down while still active used to leave the ambience suppression count above
	// zero. A loop reset does not reload the world, so that left the floor silent
	// for the rest of the session.
	StopTensionMusic();

	Super::EndPlay(EndPlayReason);
}

void UPursuerAnomalyComponent::RestoreNormalState()
{
	StopTensionMusic();

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
		StopTensionMusic();

		SpawnedPursuer = nullptr;
		// Keep the anomaly active for the rest of this floor visit even after
		// the manifestation despawns. Elevator judgment and achievements must
		// reflect what the player encountered, not whether the actor still exists.
	}
}

void UPursuerAnomalyComponent::StartTensionMusic()
{
	ClearTensionMusicDelay();

	if (!IsValid(SpawnedPursuer))
	{
		return;
	}
	if (!ActiveAnomalyLoopSound)
	{
		ActiveAnomalyLoopSound = LoadObject<USoundBase>(
			nullptr,
			TEXT("/Game/MyStuff/Sound/Pursuer/185993__mmleys__dynamic-space.185993__mmleys__dynamic-space"));
	}
	if (!ActiveAnomalyLoopSound)
	{
		ActiveAnomalyLoopSound = LoadObject<USoundBase>(
			nullptr,
			TEXT("/Game/MyStuff/Sound/Pursuer/PursuerTensionLoop.PursuerTensionLoop"));
	}
	if (!ActiveAnomalyLoopSound)
	{
		ActiveAnomalyLoopSound = LoadObject<USoundBase>(
			nullptr,
			TEXT("/Game/MyStuff/Sound/MainMenu/HorrorAmbientSound.HorrorAmbientSound"));
	}
	// Guarded because the count is shared: suppressing twice without an
	// intervening stop would leave the floor silent after the chase ended.
	if (bReplaceLevelAmbience && !bAmbienceSuppressed && GetWorld())
	{
		if (UGameInstance* GI = GetWorld()->GetGameInstance())
		{
			if (ULoop9GameSettingsSubsystem* Settings = GI->GetSubsystem<ULoop9GameSettingsSubsystem>())
			{
				Settings->SuppressLevelAmbience(true);
				bAmbienceSuppressed = true;
			}
		}
	}

	if (!ActiveAnomalyLoopSound)
	{
		return;
	}

	ActiveAnomalyLoopAudioComponent = UGameplayStatics::SpawnSound2D(
		this,
		ActiveAnomalyLoopSound,
		ActiveAnomalyLoopVolume,
		1.0f,
		0.0f,
		nullptr,
		false,
		false);

	if (ActiveAnomalyLoopAudioComponent)
	{
		ActiveAnomalyLoopAudioComponent->bAutoDestroy = false;
		ActiveAnomalyLoopAudioComponent->OnAudioFinished.AddDynamic(
			this, &UPursuerAnomalyComponent::HandleTensionMusicFinished);
	}
}

void UPursuerAnomalyComponent::HandleTensionMusicFinished()
{
	if (!ActiveAnomalyLoopAudioComponent || !ActiveAnomalyLoopSound)
	{
		return;
	}

	ActiveAnomalyLoopAudioComponent->Play();
}

void UPursuerAnomalyComponent::ClearTensionMusicDelay()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TensionMusicDelayHandle);
	}
}

void UPursuerAnomalyComponent::StopTensionMusic()
{
	ClearTensionMusicDelay();
	if (ActiveAnomalyLoopAudioComponent)
	{
		ActiveAnomalyLoopAudioComponent->OnAudioFinished.RemoveDynamic(
			this, &UPursuerAnomalyComponent::HandleTensionMusicFinished);
		ActiveAnomalyLoopAudioComponent->Stop();
		ActiveAnomalyLoopAudioComponent->DestroyComponent();
		ActiveAnomalyLoopAudioComponent = nullptr;
	}

	if (bAmbienceSuppressed && GetWorld())
	{
		if (UGameInstance* GI = GetWorld()->GetGameInstance())
		{
			if (ULoop9GameSettingsSubsystem* Settings = GI->GetSubsystem<ULoop9GameSettingsSubsystem>())
			{
				Settings->SuppressLevelAmbience(false);
			}
		}
		bAmbienceSuppressed = false;
	}
}

bool UPursuerAnomalyComponent::TryGetSpawnTransformFromPoints(APawn* PlayerPawn, FTransform& OutTransform) const
{
	if (!GetWorld() || !PlayerPawn)
	{
		return false;
	}

	TArray<APursuerSpawnPoint*> Candidates;
	CollectEnabledSpawnPoints(Candidates);
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
			UE_LOG(LogLoop9, Log, TEXT("Pursuer picked spawn point '%s'"), *Chosen->GetActorNameOrLabel());
			return true;
		}

		UE_LOG(LogLoop9, Warning, TEXT("Pursuer spawn point '%s' is off navmesh, skipping"),
			*Chosen->GetActorNameOrLabel());
	}

	return false;
}

void UPursuerAnomalyComponent::CollectEnabledSpawnPoints(TArray<APursuerSpawnPoint*>& OutPoints) const
{
	OutPoints.Reset();

	for (APursuerSpawnPoint* Point : ManualSpawnPoints)
	{
		if (IsValid(Point) && Point->bEnabled)
		{
			OutPoints.Add(Point);
		}
	}

	if (OutPoints.Num() > 0)
	{
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APursuerSpawnPoint::StaticClass(), FoundActors);

	TArray<APursuerSpawnPoint*> AllEnabled;
	TArray<APursuerSpawnPoint*> Tagged;
	for (AActor* Actor : FoundActors)
	{
		APursuerSpawnPoint* Point = Cast<APursuerSpawnPoint>(Actor);
		if (!Point || !Point->bEnabled)
		{
			continue;
		}

		AllEnabled.Add(Point);
		if (SpawnPointTag.IsNone() || Point->ActorHasTag(SpawnPointTag))
		{
			Tagged.Add(Point);
		}
	}

	// Prefer tagged points when any exist; otherwise every enabled
	// BP_PursuerSpawnPoint counts so a forgotten tag does not silently
	// fall back to a random radius spawn.
	OutPoints = Tagged.Num() > 0 ? MoveTemp(Tagged) : MoveTemp(AllEnabled);
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
