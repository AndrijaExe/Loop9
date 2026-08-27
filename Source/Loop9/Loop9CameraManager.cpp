// Copyright Epic Games, Inc. All Rights Reserved.


#include "Loop9CameraManager.h"
#include "Camera/Loop9HorrorCameraModifier.h"

ALoop9CameraManager::ALoop9CameraManager()
{
	// set the min/max pitch
	ViewPitchMin = -70.0f;
	ViewPitchMax = 80.0f;
	DefaultModifiers.AddUnique(ULoop9HorrorCameraModifier::StaticClass());
}
