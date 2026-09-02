#include "Interaction/DoorInteractable.h"

#include "Subsystems/Loop9ObservationJournalSubsystem.h"

#include "Anomaly/DoorLockStateAnomalyComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ADoorInteractable::ADoorInteractable()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Root);
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DoorMesh->SetCollisionResponseToAllChannels(ECR_Block);

	DoorLockAnomaly = CreateDefaultSubobject<UDoorLockStateAnomalyComponent>(TEXT("DoorLockAnomaly"));

	static ConstructorHelpers::FObjectFinder<USoundBase> OpeningFinder(
		TEXT("/Game/MyStuff/Sound/Doors/DoorOpeningSound"));
	static ConstructorHelpers::FObjectFinder<USoundBase> CloseFinder(
		TEXT("/Game/MyStuff/Sound/Doors/DoorClose"));
	static ConstructorHelpers::FObjectFinder<USoundBase> CloseAltFinder(
		TEXT("/Game/MyStuff/Sound/Doors/431118__inspectorj__door-front-closing-a"));
	static ConstructorHelpers::FObjectFinder<USoundBase> LockedFinder(
		TEXT("/Game/MyStuff/Sound/Doors/DoorLocked"));
	static ConstructorHelpers::FObjectFinder<USoundBase> LockedAltFinder(
		TEXT("/Game/MyStuff/Sound/Doors/321087__benjaminnelan__door-locked"));
	static ConstructorHelpers::FObjectFinder<USoundBase> BlockedFinder(
		TEXT("/Game/MyStuff/Sound/Doors/DoorBlocked"));
	static ConstructorHelpers::FObjectFinder<USoundBase> BlockedAltFinder(
		TEXT("/Game/MyStuff/Sound/Doors/475850__alfonsseelen__car_door_slam_flat_block_overvecht"));

	if (OpeningFinder.Succeeded())
	{
		OpenSound = OpeningFinder.Object;
	}
	if (CloseFinder.Succeeded())
	{
		CloseSound = CloseFinder.Object;
	}
	else if (CloseAltFinder.Succeeded())
	{
		CloseSound = CloseAltFinder.Object;
	}
	if (LockedFinder.Succeeded())
	{
		LockedSound = LockedFinder.Object;
	}
	else if (LockedAltFinder.Succeeded())
	{
		LockedSound = LockedAltFinder.Object;
	}
	if (BlockedFinder.Succeeded())
	{
		BlockedSound = BlockedFinder.Object;
	}
	else if (BlockedAltFinder.Succeeded())
	{
		BlockedSound = BlockedAltFinder.Object;
	}
}

void ADoorInteractable::BeginPlay()
{
	Super::BeginPlay();

	if (!OpenSound)
	{
		OpenSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/DoorOpeningSound.DoorOpeningSound"));
	}
	if (!CloseSound)
	{
		CloseSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/DoorClose.DoorClose"));
	}
	if (!CloseSound)
	{
		CloseSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/431118__inspectorj__door-front-closing-a.431118__inspectorj__door-front-closing-a"));
	}
	if (!LockedSound)
	{
		LockedSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/DoorLocked.DoorLocked"));
	}
	if (!LockedSound)
	{
		LockedSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/321087__benjaminnelan__door-locked.321087__benjaminnelan__door-locked"));
	}
	if (!BlockedSound)
	{
		BlockedSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/DoorBlocked.DoorBlocked"));
	}
	if (!BlockedSound)
	{
		BlockedSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/475850__alfonsseelen__car_door_slam_flat_block_overvecht.475850__alfonsseelen__car_door_slam_flat_block_overvecht"));
	}

	if (DoorMesh)
	{
		ClosedRelativeRotation = DoorMesh->GetRelativeRotation();
		OpenRelativeRotation = ClosedRelativeRotation + FRotator(0.0f, OpenYawAngle, 0.0f);
		TargetRelativeRotation = ClosedRelativeRotation;
	}
}

void ADoorInteractable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!DoorMesh)
	{
		return;
	}

	if (RattleTimeRemaining > 0.0f)
	{
		TickRattle(DeltaSeconds);
		return;
	}

	if (!bIsMoving)
	{
		return;
	}

	const FRotator Current = DoorMesh->GetRelativeRotation();
	const FRotator Next = FMath::RInterpConstantTo(Current, TargetRelativeRotation, DeltaSeconds, OpenCloseSpeedDegPerSec);
	DoorMesh->SetRelativeRotation(Next);

	if (Next.Equals(TargetRelativeRotation, 0.5f))
	{
		DoorMesh->SetRelativeRotation(TargetRelativeRotation);
		bIsMoving = false;
		SetActorTickEnabled(false);
	}
}

bool ADoorInteractable::Interact()
{
	if (bIsBlocked || bIsLocked)
	{
		PlayDoorSound(ResolveDeniedSound());
		StartRattle();
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (ULoop9ObservationJournalSubsystem* Journal =
				GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
			{
				Journal->RecordDoorDenied();
			}
		}
		return false;
	}

	bIsOpen = !bIsOpen;
	TargetRelativeRotation = bIsOpen ? OpenRelativeRotation : ClosedRelativeRotation;
	bIsMoving = true;
	SetActorTickEnabled(true);

	PlayDoorSound(bIsOpen ? ResolveOpenSound() : ResolveCloseSound());
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoop9ObservationJournalSubsystem* Journal =
			GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
		{
			if (bIsOpen)
			{
				Journal->RecordDoorOpened();
			}
			else
			{
				Journal->RecordDoorClosed();
			}
		}
	}
	return true;
}

void ADoorInteractable::SetLocked(bool bLocked)
{
	bIsLocked = bLocked;
}

void ADoorInteractable::SetBlocked(bool bBlocked)
{
	bIsBlocked = bBlocked;
}

void ADoorInteractable::LockDoor()
{
	bIsLocked = true;
}

void ADoorInteractable::UnlockDoor()
{
	bIsLocked = false;
}

bool ADoorInteractable::TryInteract_Implementation(APlayerController* InteractingController)
{
	return Interact();
}

FText ADoorInteractable::GetInteractionPromptText_Implementation() const
{
	if (bIsBlocked)
	{
		return NSLOCTEXT("Loop9Interaction", "DoorBlocked", "Blocked by something behind");
	}

	if (bIsLocked)
	{
		return NSLOCTEXT("Loop9Interaction", "DoorLocked", "Locked");
	}

	return bIsOpen
		? NSLOCTEXT("Loop9Interaction", "DoorClose", "Close")
		: NSLOCTEXT("Loop9Interaction", "DoorOpen", "Open");
}

void ADoorInteractable::PlayDoorSound(USoundBase* SoundToPlay)
{
	if (!SoundToPlay)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());
}

USoundBase* ADoorInteractable::ResolveOpenSound() const
{
	return OpenSound.Get();
}

USoundBase* ADoorInteractable::ResolveCloseSound() const
{
	return CloseSound ? CloseSound.Get() : OpenSound.Get();
}

void ADoorInteractable::StartRattle()
{
	// Swinging and shuddering at once would fight over the same rotation, and a
	// door that is mid-swing is not the one refusing to open.
	if (!bRattleWhenDenied || !DoorMesh || bIsMoving || RattleAngleDeg <= 0.0f)
	{
		return;
	}

	RattleTimeRemaining = RattleDurationSeconds;
	SetActorTickEnabled(true);
}

void ADoorInteractable::TickRattle(float DeltaSeconds)
{
	RattleTimeRemaining = FMath::Max(0.0f, RattleTimeRemaining - DeltaSeconds);

	if (RattleTimeRemaining <= 0.0f)
	{
		DoorMesh->SetRelativeRotation(ClosedRelativeRotation);
		if (!bIsMoving)
		{
			SetActorTickEnabled(false);
		}
		return;
	}

	// Amplitude falls off with the remaining time so the door settles instead of
	// stopping mid-shudder.
	const float Remaining = RattleTimeRemaining / FMath::Max(RattleDurationSeconds, KINDA_SMALL_NUMBER);
	const float Elapsed = RattleDurationSeconds - RattleTimeRemaining;
	const float Offset = FMath::Sin(Elapsed * RattleShakesPerSecond * 2.0f * PI) * RattleAngleDeg * Remaining;

	DoorMesh->SetRelativeRotation(ClosedRelativeRotation + FRotator(0.0f, Offset, 0.0f));
}

USoundBase* ADoorInteractable::ResolveDeniedSound() const
{
	if (bIsBlocked && BlockedSound)
	{
		return BlockedSound.Get();
	}
	return LockedSound.Get();
}
