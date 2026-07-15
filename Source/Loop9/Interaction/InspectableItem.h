#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Loop9Interactable.h"
#include "InspectableItem.generated.h"

class UInspectableComponent;
class UStaticMeshComponent;

/**
 * Ready-made inspectable prop: place it in the level, assign a static mesh,
 * and the player can pick it up for a close-up look (Resident Evil style).
 *
 * For custom actors, add UInspectableComponent instead and call
 * StartInspection() from your own TryInteract.
 */
UCLASS()
class LOOP9_API AInspectableItem : public AActor, public ILoop9Interactable
{
	GENERATED_BODY()

public:
	AInspectableItem();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inspection")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inspection")
	TObjectPtr<UInspectableComponent> Inspectable;

	/** Prompt shown when the player looks at the item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection")
	FText PromptText = NSLOCTEXT("Loop9Interaction", "Inspect", "Examine");

	virtual bool TryInteract_Implementation(APlayerController* InteractingController) override;
	virtual FText GetInteractionPromptText_Implementation() const override;
};
