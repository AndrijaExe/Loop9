#include "Anomaly/Pursuer/PursuerAnomalyCharacter.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Anomaly/Pursuer/PursuerAnomalyAIController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

APursuerAnomalyCharacter::APursuerAnomalyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
   AIControllerClass = APursuerAnomalyAIController::StaticClass();
}

void APursuerAnomalyCharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnDefaultController();
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
		CatchAndDespawn();
		return;
	}

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		const float DistToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
		if (DistToPlayer <= CatchDistance)
		{
			CatchAndDespawn();
			return;
		}
	}

 MoveRefreshAccumulator += DeltaSeconds;

	if (bOnlyChaseWhenNotObserved)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
            bIsObservedByPlayer = ComputeIsObservedByPlayer(PlayerPawn);
			ApplyObservationFreeze(bIsObservedByPlayer);

			if (APursuerAnomalyAIController* PursuerAI = Cast<APursuerAnomalyAIController>(GetController()))
			{
				PursuerAI->SetTargetActor(PlayerPawn);
				PursuerAI->SetObservationState(bIsObservedByPlayer);
			}

			if (bIsObservedByPlayer)
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

void APursuerAnomalyCharacter::UpdateChase()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
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

bool APursuerAnomalyCharacter::ComputeIsObservedByPlayer(APawn* PlayerPawn) const
{
	if (!PlayerPawn)
	{
		return false;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}

	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = PC->PlayerCameraManager->GetActorForwardVector().GetSafeNormal();
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

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->bPauseAnims = bObservedNow;
	}
}

void APursuerAnomalyCharacter::CatchAndDespawn()
{
	if (bHasCaughtPlayer)
	{
		return;
	}

	bHasCaughtPlayer = true;

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

	if (bShouldPlayMurmur && !bMurmurPlaying)
	{
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
			true);

		if (MovingMurmurAudioComponent)
		{
			MovingMurmurAudioComponent->bAutoDestroy = false;
			bMurmurPlaying = true;
		}
	}
	else if (!bShouldPlayMurmur && bMurmurPlaying)
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
