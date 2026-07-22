#include "Anomaly/AnomalyComponent.h"

#include "Components/PrimitiveComponent.h"

UAnomalyComponent::UAnomalyComponent()
{
	AnomalyProbability = 0.5f;
}

void UAnomalyComponent::BeginPlay()
{
	Super::BeginPlay();
	CaptureBaselineState();
}

void UAnomalyComponent::CaptureBaselineState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	bOwnerHiddenInGame = Owner->IsHidden();
	bOwnerCollisionEnabled = Owner->GetActorEnableCollision();
	OwnerScale = Owner->GetActorScale3D();

	PrimitiveBaselineStates.Reset();
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	PrimitiveBaselineStates.Reserve(PrimitiveComponents.Num());
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		FPrimitiveBaselineState& State = PrimitiveBaselineStates.AddDefaulted_GetRef();
		State.Component = PrimitiveComponent;
		State.bVisible = PrimitiveComponent->IsVisible();
		State.bHiddenInGame = PrimitiveComponent->bHiddenInGame;
		State.CollisionEnabled = PrimitiveComponent->GetCollisionEnabled();
	}

	bBaselineCaptured = true;
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

	if (!bBaselineCaptured)
	{
		CaptureBaselineState();
	}

	Owner->SetActorHiddenInGame(true);
	Owner->SetActorEnableCollision(false);
	Owner->SetActorScale3D(FVector(0.001f, 0.001f, 0.001f));

	for (const FPrimitiveBaselineState& State : PrimitiveBaselineStates)
	{
		if (UPrimitiveComponent* PrimitiveComponent = State.Component.Get())
		{
			PrimitiveComponent->SetVisibility(false, false);
			PrimitiveComponent->SetHiddenInGame(true);
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	return true;
}

void UAnomalyComponent::RestoreNormalState()
{
	AActor* Owner = GetOwner();
	if (!Owner || !bBaselineCaptured)
	{
		return;
	}

	Owner->SetActorHiddenInGame(bOwnerHiddenInGame);
	Owner->SetActorEnableCollision(bOwnerCollisionEnabled);
	Owner->SetActorScale3D(OwnerScale);

	for (const FPrimitiveBaselineState& State : PrimitiveBaselineStates)
	{
		if (UPrimitiveComponent* PrimitiveComponent = State.Component.Get())
		{
			PrimitiveComponent->SetVisibility(State.bVisible, false);
			PrimitiveComponent->SetHiddenInGame(State.bHiddenInGame);
			PrimitiveComponent->SetCollisionEnabled(State.CollisionEnabled);
		}
	}
}

void UAnomalyComponent::ResetToNormalState()
{
	RestoreNormalState();
}
