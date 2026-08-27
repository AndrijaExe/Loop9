#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "Loop9HorrorCameraModifier.generated.h"

/**
 * Subtle first-person lag, idle tremor, and walk bob. Skipped during
 * elevator, endings, inspection, and pause so those cameras stay authored.
 */
UCLASS()
class LOOP9_API ULoop9HorrorCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	ULoop9HorrorCameraModifier();

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

	/** Higher = snappier look while moving the mouse. */
	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float LookingLagSpeed = 12.0f;

	/** Lower = more float after the mouse stops. */
	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float IdleLagSpeed = 3.2f;

	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float BreathPitchDegrees = 0.16f;

	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float TremorPitchDegrees = 0.035f;

	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float TremorPitchFineDegrees = 0.02f;

	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float IdleYawDegrees = 0.07f;

	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float IdleRollDegrees = 0.11f;

	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float WalkBobPitchDegrees = 0.32f;

	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float WalkBobRollDegrees = 0.22f;

	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float WalkBobHeightCm = 1.15f;

	UPROPERTY(EditAnywhere, Category="Loop9 Camera")
	float IdleFovWobble = 0.16f;

private:
	bool ShouldApply(const FMinimalViewInfo& POV) const;

	FRotator LaggedRotation = FRotator::ZeroRotator;
	FRotator PreviousTargetRotation = FRotator::ZeroRotator;
	bool bLagInitialized = false;
	float IdleBlend = 0.0f;
	float SwayTime = 0.0f;
	float BobTime = 0.0f;
};
