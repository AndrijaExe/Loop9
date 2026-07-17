// Fill out your copyright notice in the Description page of Project Settings.

#include "TeleportPoint.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/GameInstance.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATeleportPoint::ATeleportPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root scene component
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create arrow component for direction visualization
	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	DirectionArrow->SetupAttachment(RootComponent);
	DirectionArrow->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	DirectionArrow->ArrowSize = 2.0f;

	// Create billboard component for editor visualization
	IconBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("IconBillboard"));
	IconBillboard->SetupAttachment(RootComponent);
	IconBillboard->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

	// Default type
	TeleportType = ECustomTeleportType::Entry;
}

void ATeleportPoint::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>())
		{
			LoopManager->RegisterTeleportPoint(this);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("TeleportPoint '%s' initialized as %s"),
		*GetName(),
		TeleportType == ECustomTeleportType::Entry ? TEXT("Entry") : TEXT("Exit"));
}

void ATeleportPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>())
		{
			LoopManager->UnregisterTeleportPoint(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ATeleportPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FVector ATeleportPoint::GetTeleportLocation() const
{
	return GetActorLocation();
}

FRotator ATeleportPoint::GetTeleportRotation() const
{
	return GetActorRotation();
}

#if WITH_EDITOR
void ATeleportPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		FName PropertyName = PropertyChangedEvent.Property->GetFName();

		// Change arrow color based on type
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ATeleportPoint, TeleportType))
		{
			if (DirectionArrow)
			{
				FColor ArrowColor = (TeleportType == ECustomTeleportType::Entry) ? FColor::Blue : FColor::Green;
				DirectionArrow->SetArrowColor(ArrowColor);
			}
		}
	}
}
#endif