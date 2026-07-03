// Copyright Epic Games, Inc. All Rights Reserved.

#include "Loop9GameMode.h"
#include "Controllers/Loop9PlayerController.h"

ALoop9GameMode::ALoop9GameMode()
{
	PlayerControllerClass = ALoop9PlayerController::StaticClass();
}
