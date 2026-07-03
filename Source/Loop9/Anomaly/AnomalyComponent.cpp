#include "Anomaly/AnomalyComponent.h"

#include "Components/BrushComponent.h"
#include "Components/PrimitiveComponent.h"

UAnomalyComponent::UAnomalyComponent()
{
	AnomalyProbability = 0.5f;
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

bool UAnomalyComponent::ApplyAnomalyState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	if (UBrushComponent* BrushComp = Owner->FindComponentByClass<UBrushComponent>())
	{
		BrushComp->SetVisibility(false, true);
		BrushComp->SetHiddenInGame(true);
		BrushComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	Owner->SetActorHiddenInGame(true);
	Owner->SetActorEnableCollision(false);
	Owner->SetActorScale3D(FVector(0.001f, 0.001f, 0.001f));

	TArray<UPrimitiveComponent*> PrimComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimComponents);
	for (UPrimitiveComponent* PrimComp : PrimComponents)
	{
		PrimComp->SetVisibility(false, true);
		PrimComp->SetHiddenInGame(true);
		PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	return true;
}

void UAnomalyComponent::RestoreNormalState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (UBrushComponent* BrushComp = Owner->FindComponentByClass<UBrushComponent>())
	{
		BrushComp->SetVisibility(true, true);
		BrushComp->SetHiddenInGame(false);
		BrushComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	Owner->SetActorHiddenInGame(false);
	Owner->SetActorEnableCollision(true);
	Owner->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));

	TArray<UPrimitiveComponent*> PrimComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimComponents);
	for (UPrimitiveComponent* PrimComp : PrimComponents)
	{
		PrimComp->SetVisibility(true, true);
		PrimComp->SetHiddenInGame(false);
		PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

void UAnomalyComponent::ResetToNormalState()
{
	RestoreNormalState();
}
