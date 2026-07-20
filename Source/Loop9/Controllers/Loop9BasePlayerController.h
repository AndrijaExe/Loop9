#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Loop9BasePlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

UCLASS(Abstract)
class LOOP9_API ALoop9BasePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Exposes the protected PlayerController flag for systems that must keep the camera updating while paused (e.g. item inspection). */
	bool GetShouldPerformFullTickWhenPaused() const { return bShouldPerformFullTickWhenPaused; }
	void SetShouldPerformFullTickWhenPaused(bool bEnable) { bShouldPerformFullTickWhenPaused = bEnable; }

protected:
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Gameplay", meta = (ClampMin = "50.0"))
	float InteractionPromptDistance = 300.0f;

	/** Complex interaction prompt traces run at this rate (Hz). Pause still clears immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Gameplay", meta = (ClampMin = "1.0", ClampMax = "60.0"))
	float InteractionPromptTraceHz = 20.0f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	virtual class UUserWidget* GetInteractionPromptWidget() const PURE_VIRTUAL(ALoop9BasePlayerController::GetInteractionPromptWidget, return nullptr;);
	void ClearInteractionPrompt();

private:
	void UpdateInteractionPrompt();
	FText ResolvePromptForActor(AActor* HitActor) const;
	void PushPromptToUI(const FText& PromptText, bool bVisible);

	FText LastPromptText;
	bool bLastPromptVisible = false;
	float InteractionPromptTraceAccumulator = 0.0f;
};
