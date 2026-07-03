// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Loop9Interactable.h"
#include "LiftButton.generated.h"

// Enum to define button type
UENUM(BlueprintType)
enum class ELiftButtonType : uint8
{
	Increment UMETA(DisplayName = "Increment (Advance Loop)"),
	Reset UMETA(DisplayName = "Reset Loop")
};

UCLASS()
class LOOP9_API ALiftButton : public AActor, public ILoop9Interactable
{
	GENERATED_BODY()
	
public:	
	ALiftButton();

protected:
	virtual void BeginPlay() override;

public:	
	// Static Mesh for the button
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ButtonMesh;

	// Type of button (Increment or Reset)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	ELiftButtonType ButtonType;

	// Interact function called when player presses E
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact();

	// Visual feedback when interacted
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteracted();

	virtual bool TryInteract_Implementation(APlayerController* InteractingController) override;
	virtual FText GetInteractionPromptText_Implementation() const override;
};
