#include "Camera/Loop9HorrorCameraModifier.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Interaction/InspectionStageActor.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/LoopManagerSubsystem.h"

ULoop9HorrorCameraModifier::ULoop9HorrorCameraModifier()
{
	Priority = 80;
}

bool ULoop9HorrorCameraModifier::ShouldApply(const FMinimalViewInfo& POV) const
{
	(void)POV;
	const APlayerCameraManager* Manager = CameraOwner;
	const APlayerController* PC = Manager ? Manager->GetOwningPlayerController() : nullptr;
	if (!PC || !PC->GetPawn())
	{
		return false;
	}

	if (UGameplayStatics::IsGamePaused(PC->GetWorld()) || PC->IsLookInputIgnored())
	{
		return false;
	}

	if (AInspectionStageActor::IsInspectionActive())
	{
		return false;
	}

	if (const UGameInstance* GameInstance = PC->GetGameInstance())
	{
		if (const ULoopManagerSubsystem* LoopManager = GameInstance->GetSubsystem<ULoopManagerSubsystem>())
		{
			if (LoopManager->IsElevatorTransitionPending() || LoopManager->bGameFinished)
			{
				return false;
			}
		}
	}

	return true;
}

bool ULoop9HorrorCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	if (!ShouldApply(InOutPOV))
	{
		bLagInitialized = false;
		IdleBlend = 0.0f;
		return false;
	}

	const FRotator Target = InOutPOV.Rotation;
	if (!bLagInitialized)
	{
		LaggedRotation = Target;
		PreviousTargetRotation = Target;
		bLagInitialized = true;
	}

	const float YawDelta = FMath::Abs(FRotator::NormalizeAxis(Target.Yaw - PreviousTargetRotation.Yaw));
	const float PitchDelta = FMath::Abs(FRotator::NormalizeAxis(Target.Pitch - PreviousTargetRotation.Pitch));
	const bool bLooking = (YawDelta + PitchDelta) > 0.06f;
	const float IdleTarget = bLooking ? 0.0f : 1.0f;
	IdleBlend = FMath::FInterpTo(IdleBlend, IdleTarget, DeltaTime, bLooking ? 6.0f : 1.8f);
	PreviousTargetRotation = Target;

	// Tight while aiming, a little floaty once the mouse stops.
	const float LagSpeed = FMath::Lerp(LookingLagSpeed, IdleLagSpeed, IdleBlend);
	LaggedRotation = FMath::RInterpTo(LaggedRotation, Target, DeltaTime, LagSpeed);

	SwayTime += DeltaTime;
	const float BreathPitch = FMath::Sin(SwayTime * 1.05f) * BreathPitchDegrees;
	const float TremorPitch =
		FMath::Sin(SwayTime * 8.2f) * TremorPitchDegrees + FMath::Sin(SwayTime * 13.7f) * TremorPitchFineDegrees;
	const float IdleYaw = FMath::Sin(SwayTime * 0.47f) * IdleYawDegrees;
	const float IdleRoll = FMath::Sin(SwayTime * 0.63f) * IdleRollDegrees;
	const FRotator IdleOffset(BreathPitch + TremorPitch, IdleYaw, IdleRoll);

	float WalkAlpha = 0.0f;
	if (const APlayerController* PC = CameraOwner ? CameraOwner->GetOwningPlayerController() : nullptr)
	{
		if (const ACharacter* Character = Cast<ACharacter>(PC->GetPawn()))
		{
			const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
			if (Movement && Movement->IsMovingOnGround())
			{
				WalkAlpha = FMath::Clamp(Character->GetVelocity().Size2D() / 300.0f, 0.0f, 1.0f);
			}
		}
	}

	BobTime += DeltaTime * FMath::Lerp(0.0f, 7.2f, WalkAlpha);
	const float BobPitch = FMath::Sin(BobTime) * WalkBobPitchDegrees * WalkAlpha;
	const float BobRoll = FMath::Cos(BobTime * 0.5f) * WalkBobRollDegrees * WalkAlpha;
	const float BobHeight = FMath::Sin(BobTime) * WalkBobHeightCm * WalkAlpha;

	InOutPOV.Rotation = LaggedRotation + IdleOffset * IdleBlend + FRotator(BobPitch, 0.0f, BobRoll);
	InOutPOV.Location += InOutPOV.Rotation.RotateVector(FVector(0.0f, 0.0f, BobHeight));
	InOutPOV.FOV += FMath::Sin(SwayTime * 0.55f) * IdleFovWobble * IdleBlend;
	return false;
}
