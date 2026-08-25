// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StreamableManager.h"
#include "GameHelpers.generated.h"

/**
 * Game Helper Functions
 */
UCLASS()
class LOOP9_API UGameHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Load level with optional loading screen delay */
	UFUNCTION(BlueprintCallable, Category = "Game", meta = (WorldContext = "WorldContextObject"))
	static void LoadLevelWithLoadingScreen(
		UObject* WorldContextObject, 
		FName LevelName,
		TSubclassOf<UUserWidget> LoadingScreenClass = nullptr
	);
};
