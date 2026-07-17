#include "Interaction/InspectableItem.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/InspectableComponent.h"

AInspectableItem::AInspectableItem()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	// Must block the visibility channel so the interact line trace can hit it.
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	Inspectable = CreateDefaultSubobject<UInspectableComponent>(TEXT("Inspectable"));
}

bool AInspectableItem::TryInteract_Implementation(APlayerController* InteractingController)
{
	return Inspectable && Inspectable->StartInspection(InteractingController);
}

FText AInspectableItem::GetInteractionPromptText_Implementation() const
{
	return Inspectable ? Inspectable->PromptText : FText::GetEmpty();
}
