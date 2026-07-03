// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Loop9BasePlayerController.h"
#include "Loop9PlayerController.generated.h"

class UBlinkOverlayWidget;
class UHorrorUI;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract)
class LOOP9_API ALoop9PlayerController : public ALoop9BasePlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	ALoop9PlayerController();

protected:
	/** Blink overlay widget class (for intro cutscene) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI|Cutscene")
	TSubclassOf<UBlinkOverlayWidget> BlinkOverlayWidgetClass;

	UPROPERTY()
	UBlinkOverlayWidget* BlinkOverlayInstance;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Possessed pawn initialization */
	virtual void OnPossess(APawn* InPawn) override;

	/** Per-frame prompt update */
	virtual UHorrorUI* GetInteractionPromptUI() const override;

	/** Optional HUD widget class for crosshair/prompt */
	UPROPERTY(EditAnywhere, Category="UI|Gameplay")
	TSubclassOf<UHorrorUI> GameplayUIClass;

	/** Optional gameplay UI instance */
	UPROPERTY(Transient)
	TObjectPtr<UHorrorUI> GameplayUI;

public:
	UFUNCTION(BlueprintCallable, Category="UI|Cutscene")
	void CreateBlinkOverlay();

	UFUNCTION(BlueprintCallable, Category="UI|Cutscene")
	void PlayBlink(float Duration = 0.35f);

	UFUNCTION(BlueprintCallable, Category="UI|Cutscene")
	void RemoveBlinkOverlay();

};
