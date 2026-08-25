#include "Interaction/DoorInteractable.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
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

	static ConstructorHelpers::FObjectFinder<USoundBase> OpenFinder(
		TEXT("/Game/MyStuff/Sound/Doors/DoorOpen"));
	static ConstructorHelpers::FObjectFinder<USoundBase> CloseFinder(
		TEXT("/Game/MyStuff/Sound/Doors/DoorClose"));
	static ConstructorHelpers::FObjectFinder<USoundBase> LockedFinder(
		TEXT("/Game/MyStuff/Sound/Doors/DoorLocked"));
	static ConstructorHelpers::FObjectFinder<USoundBase> BlockedFinder(
		TEXT("/Game/MyStuff/Sound/Doors/DoorBlocked"));

	if (OpenFinder.Succeeded())
	{
		OpenSound = OpenFinder.Object;
		OpenCloseSound = OpenFinder.Object;
	}
	if (CloseFinder.Succeeded())
	{
		CloseSound = CloseFinder.Object;
	}
	if (LockedFinder.Succeeded())
	{
		LockedSound = LockedFinder.Object;
	}
	if (BlockedFinder.Succeeded())
	{
		BlockedSound = BlockedFinder.Object;
	}
}

void ADoorInteractable::BeginPlay()
{
	Super::BeginPlay();

	if (!OpenSound)
	{
		OpenSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/DoorOpen.DoorOpen"));
	}
	if (!CloseSound)
	{
		CloseSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/DoorClose.DoorClose"));
	}
	if (!LockedSound)
	{
		LockedSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/DoorLocked.DoorLocked"));
	}
	if (!BlockedSound)
	{
		BlockedSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Doors/DoorBlocked.DoorBlocked"));
	}
	if (!OpenCloseSound)
	{
		OpenCloseSound = OpenSound;
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
		return false;
	}

	bIsOpen = !bIsOpen;
	TargetRelativeRotation = bIsOpen ? OpenRelativeRotation : ClosedRelativeRotation;
	bIsMoving = true;
	SetActorTickEnabled(true);

	PlayDoorSound(bIsOpen ? ResolveOpenSound() : ResolveCloseSound());
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
	return OpenSound ? OpenSound.Get() : OpenCloseSound.Get();
}

USoundBase* ADoorInteractable::ResolveCloseSound() const
{
	if (CloseSound)
	{
		return CloseSound.Get();
	}
	return ResolveOpenSound();
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
