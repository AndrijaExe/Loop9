// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Controllers/Loop9BasePlayerController.h"
#include "HorrorPlayerController.generated.h"

class UHorrorUI;

/**
 *  Player Controller for a first person horror game
 *  Manages input mappings
 *  Manages UI
 */
UCLASS(abstract)
class LOOP9_API AHorrorPlayerController : public ALoop9BasePlayerController
{
	GENERATED_BODY()
	
protected:

	/** Type of UI widget to spawn */
	UPROPERTY(EditAnywhere, Category="Horror|UI")
	TSubclassOf<UHorrorUI> HorrorUIClass;

	/** Pointer to the UI widget */
	TObjectPtr<UHorrorUI> HorrorUI;

public:

	/** Constructor */
	AHorrorPlayerController();

protected:

	/** Gameplay Initialization */
	virtual void BeginPlay() override;

	/** Possessed pawn initialization */
	virtual void OnPossess(APawn* aPawn) override;
	virtual UHorrorUI* GetInteractionPromptUI() const override;

};
