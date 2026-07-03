// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Loop/LoopTypes.h"
#include "UI/EndingWidget.h"
#include "UI/ReplacementTerminalWidget.h"
#include "Loop9GameMode.generated.h"

UCLASS(abstract)
class LOOP9_API ALoop9GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALoop9GameMode();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UEndingWidget> EndingWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TMap<ELoopEndingType, TSubclassOf<UEndingWidget>> EndingWidgetClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UReplacementTerminalWidget> ReplacementTerminalWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class ULoop9NotificationWidget> GameplayNotificationWidgetClass;
};
