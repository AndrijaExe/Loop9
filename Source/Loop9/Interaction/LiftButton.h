#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Loop9Interactable.h"
#include "LiftButton.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UTextRenderComponent;
class ALiftDoorWing;
class ALoopElevatorTransitionDirector;

UENUM(BlueprintType)
enum class ELiftButtonType : uint8
{
	Increment UMETA(DisplayName = "Next Floor (lights off)"),
	Reset UMETA(DisplayName = "Restart Loop (lights on)")
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ButtonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* IndicatorLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* LabelText;

	/**
	 * Optional cabin gate volume. When enabled with a non-trivial extent, the
	 * player must overlap this box to press the button. Leave disabled to use
	 * the automatic door-plane check against TransitionDoorWings.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CabinVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	ELiftButtonType ButtonType = ELiftButtonType::Increment;

	/** Reject presses when the player is outside the elevator cabin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Cabin Gate")
	bool bRequirePlayerInsideCabin = true;

	/** Use CabinVolume overlap when the box extent is configured (> 1 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Cabin Gate")
	bool bUseCabinVolumeGate = true;

	/** Minimum depth past the door plane toward the button (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Cabin Gate", meta = (ClampMin = "0.0"))
	float CabinMinDepthPastDoorsCm = 25.0f;

	/** Max distance from the button while still counting as inside the cabin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Cabin Gate", meta = (ClampMin = "50.0"))
	float CabinMaxDistanceFromButtonCm = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Visual")
	bool bEnableIndicatorLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Visual", meta = (ClampMin = "0.0"))
	float ActiveLightIntensity = 4500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Visual", meta = (ClampMin = "0.0"))
	float InactiveLightIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Visual")
	FLinearColor ActiveLightColor = FLinearColor(1.0f, 0.82f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Visual")
	FLinearColor InactiveLightColor = FLinearColor(0.15f, 0.25f, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Visual", meta = (ClampMin = "0.0"))
	float EmissiveStrengthWhenActive = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Visual", meta = (ClampMin = "0.0"))
	float EmissiveStrengthWhenInactive = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Label")
	FText WorldLabelText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Label", meta = (ClampMin = "4.0"))
	float LabelWorldSize = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Label")
	FVector LabelRelativeOffset = FVector(0.0f, 0.0f, 55.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Interaction", meta = (DisplayName = "Custom Interaction Prompt"))
	FText InteractionPromptText;

	/** Optional cinematic director. When unset, the legacy instant transition remains available. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Button|Transition")
	TObjectPtr<ALoopElevatorTransitionDirector> TransitionDirector;

	/** Door wings belonging to the elevator selected by this button. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Button|Transition")
	TArray<TObjectPtr<ALiftDoorWing>> TransitionDoorWings;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact();

	UFUNCTION(BlueprintCallable, Category = "Button|Visual")
	void ApplyIndicatorVisuals();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteracted();

	virtual bool TryInteract_Implementation(APlayerController* InteractingController) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

private:
	bool HandleInteraction(APlayerController* InteractingController);
	bool IsPlayerInsideCabin(APlayerController* InteractingController) const;
	void UpdateMeshEmissive(float Strength, const FLinearColor& Tint) const;
};
