// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UI/EndingWidget.h"
#include "UI/ReplacementTerminalWidget.h"
#include "Loop9GameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class ALoop9GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALoop9GameMode();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UEndingWidget> EndingWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UReplacementTerminalWidget> ReplacementTerminalWidgetClass;
};



