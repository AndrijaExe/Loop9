// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Loop9Character.h"
#include "HorrorCharacter.generated.h"

class USpotLightComponent;

/**
 * Extra spotlight character. Movement, sprint, and the handheld flashlight
 * live on ALoop9Character (the pawn BP_Character actually uses).
 */
UCLASS(abstract)
class LOOP9_API AHorrorCharacter : public ALoop9Character
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpotLightComponent* SpotLight;

public:
	AHorrorCharacter();
};
