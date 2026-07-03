// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HorrorUI.generated.h"

class AHorrorCharacter;
class UTextBlock;

/**
 *  Simple UI for a first person horror game
 *  Manages character sprint meter display
 */
UCLASS()
class LOOP9_API UHorrorUI : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Sets up delegate listeners for the passed character */
	void SetupCharacter(AHorrorCharacter* HorrorCharacter);

	/** Updates interaction prompt text/visibility */
	void SetInteractionPrompt(const FText& PromptText, bool bVisible);

	/** Sets crosshair visibility */
	void SetCrosshairVisible(bool bVisible);

	/** Called when the character's sprint meter is updated */
	UFUNCTION()
	void OnSprintMeterUpdated(float Percent);

	/** Called when the character's sprint state changes */
	UFUNCTION()
	void OnSprintStateChanged(bool bSprinting);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_InteractionPrompt;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_CrosshairDot;

	/** Passes control to Blueprint to update the sprint meter widgets */
	UFUNCTION(BlueprintImplementableEvent, Category="Horror", meta = (DisplayName = "Sprint Meter Updated"))
	void BP_SprintMeterUpdated(float Percent);

	/** Passes control to Blueprint to update the sprint meter status */
	UFUNCTION(BlueprintImplementableEvent, Category="Horror", meta = (DisplayName = "Sprint State Changed"))
	void BP_SprintStateChanged(bool bSprinting);

	/** Passes control to Blueprint to update interaction prompt */
	UFUNCTION(BlueprintImplementableEvent, Category="Horror", meta = (DisplayName = "Interaction Prompt Updated"))
	void BP_InteractionPromptUpdated(const FText& PromptText, bool bVisible);

	/** Passes control to Blueprint to toggle crosshair */
	UFUNCTION(BlueprintImplementableEvent, Category="Horror", meta = (DisplayName = "Crosshair Visibility Changed"))
	void BP_CrosshairVisibilityChanged(bool bVisible);
};
