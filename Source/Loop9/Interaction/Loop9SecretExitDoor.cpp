#include "Interaction/Loop9SecretExitDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Subsystems/Loop9ObservationJournalSubsystem.h"
#include "Subsystems/LoopManagerSubsystem.h"

ALoop9SecretExitDoor::ALoop9SecretExitDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	// Must block the visibility channel so the interact line trace can hit it.
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

bool ALoop9SecretExitDoor::TryInteract_Implementation(APlayerController* InteractingController)
{
	if (bConsumed)
	{
		return true;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULoopManagerSubsystem* LoopManager = GameInstance ? GameInstance->GetSubsystem<ULoopManagerSubsystem>() : nullptr;

	if (ULoop9ObservationJournalSubsystem* Journal =
		GameInstance ? GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>() : nullptr)
	{
		Journal->RecordObjectInspected(FName(TEXT("ground_floor_door")));
	}

	if (LoopManager && LoopManager->TryTriggerSecretExitEnding(HiddenWallActor))
	{
		bConsumed = true;
		if (OpenSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this, OpenSound, GetActorLocation(), GetActorRotation(), 1.0f, 1.0f, 0.0f, Attenuation);
		}
		OnExitAccepted();
		return true;
	}

	if (LockedSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, LockedSound, GetActorLocation(), GetActorRotation(), 1.0f, 1.0f, 0.0f, Attenuation);
	}
	OnExitRefused();
	return true;
}

FText ALoop9SecretExitDoor::GetInteractionPromptText_Implementation() const
{
	return NSLOCTEXT("Loop9Interaction", "OpenStreetDoor", "Open");
}
