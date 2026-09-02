#include "Anomaly/Pursuer/PursuerAnomalyCharacter.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Anomaly/Pursuer/PursuerAnomalyAIController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Subsystems/Loop9ObservationJournalSubsystem.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

APursuerAnomalyCharacter::APursuerAnomalyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = APursuerAnomalyAIController::StaticClass();

	static ConstructorHelpers::FObjectFinder<USoundBase> MurmurFinder(
		TEXT("/Game/MyStuff/Sound/Phone/MumblingCrazy"));
	if (MurmurFinder.Succeeded())
	{
		MovingMurmurLoopSound = MurmurFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundAttenuation> MurmurAttenuationFinder(
		TEXT("/Game/MyStuff/Sound/Footsteps/ATT_PursuerFootstep"));
	if (MurmurAttenuationFinder.Succeeded())
	{
		MovingMurmurAttenuation = MurmurAttenuationFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> DespawnFinder(
		TEXT("/Game/MyStuff/Sound/Pursuer/PursuerDespawn"));
	if (DespawnFinder.Succeeded())
	{
		DespawnSound = DespawnFinder.Object;
	}
}

void APursuerAnomalyCharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnDefaultController();
	CachedPlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	CachedPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ObservationCheckAccumulator = FMath::Max(0.01f, ObservationCheckInterval);

	if (!MovingMurmurLoopSound)
	{
		MovingMurmurLoopSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Phone/MumblingCrazy.MumblingCrazy"));
	}
	if (!MovingMurmurAttenuation)
	{
		MovingMurmurAttenuation = LoadObject<USoundAttenuation>(nullptr, TEXT("/Game/MyStuff/Sound/Footsteps/ATT_PursuerFootstep.ATT_PursuerFootstep"));
	}
	if (!DespawnSound)
	{
		DespawnSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Pursuer/PursuerDespawn.PursuerDespawn"));
	}
}

void APursuerAnomalyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bHasCaughtPlayer)
	{
		return;
	}

	LifetimeElapsed += DeltaSeconds;
	if (MaxLifetime > 0.0f && LifetimeElapsed >= MaxLifetime)
	{
		CatchAndDespawn(false);
		return;
	}

	APawn* PlayerPawn = ResolvePlayerPawn();
	if (PlayerPawn)
	{
		const float DistSquared = FVector::DistSquared(GetActorLocation(), PlayerPawn->GetActorLocation());
		if (DistSquared <= FMath::Square(CatchDistance))
		{
			CatchAndDespawn(true);
			return;
		}
	}

	MoveRefreshAccumulator += DeltaSeconds;

	if (bOnlyChaseWhenNotObserved)
	{
		ObservationCheckAccumulator += DeltaSeconds;
		const float ObservationInterval = FMath::Max(0.01f, ObservationCheckInterval);
		if (ObservationCheckAccumulator >= ObservationInterval)
		{
			ObservationCheckAccumulator = 0.0f;
			if (PlayerPawn)
			{
				bIsObservedByPlayer = ComputeIsObservedByPlayer(PlayerPawn, ResolvePlayerController());
			}
			else
			{
				bIsObservedByPlayer = false;
			}
		}

		ApplyObservationFreeze(bIsObservedByPlayer);

		if (APursuerAnomalyAIController* PursuerAI = Cast<APursuerAnomalyAIController>(GetController()))
		{
			PursuerAI->SetTargetActor(PlayerPawn);
			PursuerAI->SetObservationState(bIsObservedByPlayer);
		}

		if (PlayerPawn && bIsObservedByPlayer)
		{
			if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
			{
				MoveComp->StopMovementImmediately();
			}

			if (APursuerAnomalyAIController* PursuerAI = Cast<APursuerAnomalyAIController>(GetController()))
			{
				PursuerAI->StopChaseMovement();
			}
		}
	}
	else
	{
		bIsObservedByPlayer = false;
		ApplyObservationFreeze(false);
	}

	if (MoveRefreshAccumulator >= MoveRefreshInterval)
	{
		MoveRefreshAccumulator = 0.0f;
		UpdateChase();
	}

	UpdateMovingAudio();
}

APawn* APursuerAnomalyCharacter::ResolvePlayerPawn()
{
	if (APlayerController* PlayerController = ResolvePlayerController())
	{
		if (PlayerController->GetPawn() != CachedPlayerPawn.Get())
		{
			CachedPlayerPawn = PlayerController->GetPawn();
		}
	}
	else if (!CachedPlayerPawn.IsValid())
	{
		CachedPlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	}
	return CachedPlayerPawn.Get();
}

APlayerController* APursuerAnomalyCharacter::ResolvePlayerController()
{
	if (!CachedPlayerController.IsValid())
	{
		CachedPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}
	return CachedPlayerController.Get();
}

void APursuerAnomalyCharacter::UpdateChase()
{
	APawn* PlayerPawn = ResolvePlayerPawn();
	if (!PlayerPawn)
	{
		return;
	}

	if (bOnlyChaseWhenNotObserved && bIsObservedByPlayer)
	{
		if (APursuerAnomalyAIController* PursuerAI = Cast<APursuerAnomalyAIController>(GetController()))
		{
			PursuerAI->StopChaseMovement();
		}
		return;
	}

	if (APursuerAnomalyAIController* PursuerAI = Cast<APursuerAnomalyAIController>(GetController()))
	{
		if (PursuerAI->IsUsingBehaviorTree())
		{
			return;
		}
	}

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->MoveToActor(PlayerPawn, MoveAcceptanceRadius);
	}
}

bool APursuerAnomalyCharacter::ComputeIsObservedByPlayer(APawn* PlayerPawn, APlayerController* PlayerController) const
{
	if (!PlayerPawn || !PlayerController || !PlayerController->PlayerCameraManager)
	{
		return false;
	}

	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = PlayerController->PlayerCameraManager->GetActorForwardVector().GetSafeNormal();
	const FVector ToPursuer = (GetActorLocation() - CameraLocation).GetSafeNormal();

	const float Dot = FVector::DotProduct(CameraForward, ToPursuer);
	if (Dot < PlayerLookDotThreshold)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PursuerObservedTrace), true, PlayerPawn);

	const bool bHitSomething = GetWorld()->LineTraceSingleByChannel(
		Hit,
		CameraLocation,
		GetActorLocation(),
		ECC_Visibility,
		Params);

	if (!bHitSomething)
	{
		return true;
	}

	return Hit.GetActor() == this;
}

void APursuerAnomalyCharacter::ApplyObservationFreeze(bool bObservedNow)
{
	if (bObservedNow == bWasObservedByPlayer)
	{
		return;
	}

	bWasObservedByPlayer = bObservedNow;
	if (bObservedNow)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (ULoop9ObservationJournalSubsystem* Journal =
				GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
			{
				Journal->RecordPursuerObserved();
			}
		}
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->bPauseAnims = bObservedNow;
	}
}

void APursuerAnomalyCharacter::CatchAndDespawn(bool bCaughtPlayer)
{
	if (bHasCaughtPlayer)
	{
		return;
	}

	bHasCaughtPlayer = true;
	if (bCaughtPlayer)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (ULoop9ObservationJournalSubsystem* Journal =
				GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
			{
				Journal->RecordPursuerCaught();
			}
		}
	}

	if (MovingMurmurAudioComponent)
	{
		MovingMurmurAudioComponent->Stop();
		MovingMurmurAudioComponent->DestroyComponent();
		MovingMurmurAudioComponent = nullptr;
		bMurmurPlaying = false;
	}

	if (DespawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			DespawnSound,
			GetActorLocation(),
			GetActorRotation(),
			1.0f,
			1.0f,
			0.0f,
			DespawnSoundAttenuation,
			nullptr);
	}

	if (DespawnNiagaraEffect)
	{
       UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DespawnNiagaraEffect,
			GetActorLocation(),
			GetActorRotation());

		if (NiagaraComp && DespawnNiagaraMaxDuration > 0.0f)
		{
         NiagaraComp->SetAutoDestroy(false);

			TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComp = NiagaraComp;
			FTimerHandle NiagaraStopHandle;
			FTimerDelegate NiagaraStopDelegate;
            NiagaraStopDelegate.BindLambda([WeakNiagaraComp]()
			{
				if (WeakNiagaraComp.IsValid())
				{
					WeakNiagaraComp->Deactivate();
					WeakNiagaraComp->DestroyComponent();
				}
			});

			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(NiagaraStopHandle, NiagaraStopDelegate, DespawnNiagaraMaxDuration, false);
			}
		}
	}

	Destroy();
}

void APursuerAnomalyCharacter::UpdateMovingAudio()
{
	if (!MovingMurmurLoopSound)
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	const bool bShouldPlayMurmur = !bHasCaughtPlayer && !bIsObservedByPlayer && Speed > 10.0f;

	if (bShouldPlayMurmur)
	{
		const bool bNeedsRestart = !MovingMurmurAudioComponent || !MovingMurmurAudioComponent->IsPlaying();
		if (bNeedsRestart)
		{
			if (MovingMurmurAudioComponent)
			{
				MovingMurmurAudioComponent->Stop();
				MovingMurmurAudioComponent->DestroyComponent();
				MovingMurmurAudioComponent = nullptr;
			}

			MovingMurmurAudioComponent = UGameplayStatics::SpawnSoundAttached(
				MovingMurmurLoopSound,
				GetMesh() ? GetMesh() : RootComponent,
				NAME_None,
				FVector::ZeroVector,
				EAttachLocation::KeepRelativeOffset,
				true,
				MovingMurmurVolume,
				1.0f,
				0.0f,
				MovingMurmurAttenuation,
				nullptr,
				false);

			if (MovingMurmurAudioComponent)
			{
				MovingMurmurAudioComponent->bAutoDestroy = false;
				bMurmurPlaying = true;
			}
		}
	}
	else if (bMurmurPlaying)
	{
		if (MovingMurmurAudioComponent)
		{
			MovingMurmurAudioComponent->Stop();
			MovingMurmurAudioComponent->DestroyComponent();
			MovingMurmurAudioComponent = nullptr;
		}

		bMurmurPlaying = false;
	}
}
