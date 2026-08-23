#include "MenuSceneLibrary.h"

#include "Components/LightComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName MenuCameraTag(TEXT("MenuCamera"));
}

FVector UMenuSceneLibrary::GetLightWorldLocation(AActor* LightActor)
{
	if (!LightActor)
	{
		return FVector::ZeroVector;
	}

	TArray<ULightComponent*> Lights;
	LightActor->GetComponents<ULightComponent>(Lights);

	const ULightComponent* Brightest = nullptr;
	for (const ULightComponent* Light : Lights)
	{
		if (Light && (!Brightest || Light->Intensity > Brightest->Intensity))
		{
			Brightest = Light;
		}
	}

	return Brightest ? Brightest->GetComponentLocation() : LightActor->GetActorLocation();
}

FTransform UMenuSceneLibrary::GetFigureTransformUnderLight(
	UObject* WorldContextObject,
	AActor* LightActor,
	AActor* FaceTarget,
	float FloorClearance,
	float ForwardOffset,
	float MaxDropDistance)
{
	if (!LightActor)
	{
		return FTransform::Identity;
	}

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		World = LightActor->GetWorld();
	}

	if (!World)
	{
		return FTransform::Identity;
	}

	const FVector LightLocation = GetLightWorldLocation(LightActor);

	if (!FaceTarget)
	{
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(World, MenuCameraTag, Tagged);
		if (Tagged.Num() > 0)
		{
			FaceTarget = Tagged[0];
		}
	}

	// Drop to the floor rather than trusting the light's height: menu lights hang
	// at whatever altitude looked right, and the figure has to stand on something.
	FVector FloorLocation = LightLocation - FVector(0.0f, 0.0f, MaxDropDistance);

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(MenuFigureFloorTrace), false, LightActor);
	if (FaceTarget)
	{
		TraceParams.AddIgnoredActor(FaceTarget);
	}

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, LightLocation, FloorLocation, ECC_WorldStatic, TraceParams))
	{
		FloorLocation = Hit.ImpactPoint;
	}

	FRotator FigureRotation(0.0f, LightActor->GetActorRotation().Yaw, 0.0f);
	if (FaceTarget)
	{
		FVector ToTarget = FaceTarget->GetActorLocation() - FloorLocation;
		ToTarget.Z = 0.0f;

		if (!ToTarget.IsNearlyZero())
		{
			const FVector Forward = ToTarget.GetSafeNormal();
			FigureRotation = Forward.Rotation();
			FloorLocation += Forward * ForwardOffset;
		}
	}

	FloorLocation.Z += FloorClearance;

	return FTransform(FigureRotation, FloorLocation);
}
