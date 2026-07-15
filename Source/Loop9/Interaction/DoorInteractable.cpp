#include "Interaction/DoorInteractable.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ADoorInteractable::ADoorInteractable()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Root);
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DoorMesh->SetCollisionResponseToAllChannels(ECR_Block);
}

void ADoorInteractable::BeginPlay()
{
	Super::BeginPlay();

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

	if (!bIsMoving || !DoorMesh)
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
	}
}

bool ADoorInteractable::Interact()
{
	if (bIsLocked)
	{
		PlayDoorSound(LockedSound);
		return false;
	}

	bIsOpen = !bIsOpen;
	TargetRelativeRotation = bIsOpen ? OpenRelativeRotation : ClosedRelativeRotation;
	bIsMoving = true;

	PlayDoorSound(OpenCloseSound);
	return true;
}

void ADoorInteractable::SetLocked(bool bLocked)
{
	bIsLocked = bLocked;
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
