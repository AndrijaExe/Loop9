#include "Interaction/Loop9ObservationZoneVolume.h"

#include "Components/ShapeComponent.h"
#include "Engine/GameInstance.h"
#include "Loop9.h"
#include "Loop9Character.h"
#include "Subsystems/Loop9ObservationJournalSubsystem.h"

ALoop9ObservationZoneVolume::ALoop9ObservationZoneVolume()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(true);
}

void ALoop9ObservationZoneVolume::BeginPlay()
{
	Super::BeginPlay();

	if (ZoneId.IsNone())
	{
		UE_LOG(LogLoop9, Warning, TEXT("ObservationZoneVolume '%s' has no ZoneId."),
			*GetActorNameOrLabel());
	}
	else if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoop9ObservationJournalSubsystem* Journal =
			GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
		{
			Journal->RegisterZoneVolume(this, ZoneId);
		}
	}

	if (UShapeComponent* TriggerComponent = GetCollisionComponent())
	{
		TriggerComponent->OnComponentBeginOverlap.AddDynamic(
			this, &ALoop9ObservationZoneVolume::HandleComponentEntered);
		TriggerComponent->OnComponentEndOverlap.AddDynamic(
			this, &ALoop9ObservationZoneVolume::HandleComponentExited);
	}
}

void ALoop9ObservationZoneVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoop9ObservationJournalSubsystem* Journal =
			GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
		{
			Journal->UnregisterZoneVolume(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ALoop9ObservationZoneVolume::HandleComponentEntered(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32,
	bool,
	const FHitResult&)
{
	const ALoop9Character* Character = Cast<ALoop9Character>(OtherActor);
	if (!Character || OtherComponent != Character->GetCapsuleComponent())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoop9ObservationJournalSubsystem* Journal =
			GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
		{
			Journal->NotifyPlayerEnteredZone(this);
		}
	}
}

void ALoop9ObservationZoneVolume::HandleComponentExited(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32)
{
	const ALoop9Character* Character = Cast<ALoop9Character>(OtherActor);
	if (!Character || OtherComponent != Character->GetCapsuleComponent())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoop9ObservationJournalSubsystem* Journal =
			GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
		{
			Journal->NotifyPlayerExitedZone(this);
		}
	}
}
