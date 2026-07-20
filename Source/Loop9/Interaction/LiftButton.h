#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Loop9Interactable.h"
#include "LiftButton.generated.h"

class UPointLightComponent;
class UTextRenderComponent;
class ALiftDoorWing;
class ALoopElevatorTransitionDirector;
class ULevelSequence;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	ELiftButtonType ButtonType = ELiftButtonType::Increment;

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

	/** Optional source-elevator camera/audio sequence; overrides the director default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button|Transition")
	TObjectPtr<ULevelSequence> TransitionSequenceOverride;

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
	void UpdateMeshEmissive(float Strength, const FLinearColor& Tint) const;
};
