// Fill out your copyright notice in the Description page of Project Settings.

#include "AnomalyComponent.h"
#include "Subsystems/AnomalyManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BrushComponent.h"

UAnomalyComponent::UAnomalyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Default probability
	AnomalyProbability = 0.5f;
	bIsAnomalyActive = false;
}

void UAnomalyComponent::BeginPlay()
{
	Super::BeginPlay();

	// Register with AnomalyManager
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
	if (GameInstance)
	{
		UAnomalyManager* AnomalyManager = GameInstance->GetSubsystem<UAnomalyManager>();
		if (AnomalyManager)
		{
			AnomalyManager->RegisterAnomaly(GetOwner());
		}
	}
}

void UAnomalyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAnomalyComponent::ActivateAnomaly()
{
	if (bIsAnomalyActive)
	{
		return;
	}

	bIsAnomalyActive = true;

	// Hide the actor
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// Try to find BrushComponent (for BSP Brushes)
		UBrushComponent* BrushComp = Owner->FindComponentByClass<UBrushComponent>();
		if (BrushComp)
		{
			BrushComp->SetVisibility(false, true);
			BrushComp->SetHiddenInGame(true);
			BrushComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		// Standard hiding methods
		Owner->SetActorHiddenInGame(true);
		Owner->SetActorEnableCollision(false);
		Owner->SetActorScale3D(FVector(0.001f, 0.001f, 0.001f)); // Almost zero

		// Hide ALL primitive components
		TArray<UPrimitiveComponent*> PrimComponents;
		Owner->GetComponents<UPrimitiveComponent>(PrimComponents);
		for (UPrimitiveComponent* PrimComp : PrimComponents)
		{
			PrimComp->SetVisibility(false, true);
			PrimComp->SetHiddenInGame(true);
			PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	// Broadcast event
	OnAnomalyActivated.Broadcast(true);
}

void UAnomalyComponent::DeactivateAnomaly()
{
	if (!bIsAnomalyActive)
	{
		return;
	}

	bIsAnomalyActive = false;

	// Show the actor
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// Try to find BrushComponent (for BSP Brushes)
		UBrushComponent* BrushComp = Owner->FindComponentByClass<UBrushComponent>();
		if (BrushComp)
		{
			BrushComp->SetVisibility(true, true);
			BrushComp->SetHiddenInGame(false);
			BrushComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}

		// Standard showing methods
		Owner->SetActorHiddenInGame(false);
		Owner->SetActorEnableCollision(true);
		Owner->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));

		// Show ALL primitive components
		TArray<UPrimitiveComponent*> PrimComponents;
		Owner->GetComponents<UPrimitiveComponent>(PrimComponents);
		for (UPrimitiveComponent* PrimComp : PrimComponents)
		{
			PrimComp->SetVisibility(true, true);
			PrimComp->SetHiddenInGame(false);
			PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}

	// Broadcast event
	OnAnomalyDeactivated.Broadcast(false);
}

void UAnomalyComponent::ToggleAnomaly()
{
	if (bIsAnomalyActive)
	{
		DeactivateAnomaly();
	}
	else
	{
		ActivateAnomaly();
	}
}

