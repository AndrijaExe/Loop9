#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Loop9TrailerRigSubsystem.generated.h"

class APlayerController;

/**
 * Trailer capture rig: repeatable camera moves through the real player
 * camera, so every clip is genuine gameplay (the horror camera modifier's
 * lag, tremor and walk bob stay on) but nothing is done by hand.
 *
 * Walk to a spot, `TrailerMark clip2`; later `TrailerShot clip2 pan 25 time 5`
 * teleports the pawn back there and pans 25 degrees over 5 s with an eased
 * curve, mouse and movement input ignored for the duration. Running the same
 * shot before and after an anomaly gives the identical framing the
 * spot-the-difference cut needs. Marks persist in Saved/Trailer/Marks.ini.
 *
 * Driven from the ALoop9PlayerController Trailer* console commands; those are
 * compiled out of Shipping, this class is harmless there.
 */
UCLASS()
class LOOP9_API ULoop9TrailerRigSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Saves the pawn's location and the control rotation under Name. */
	bool MarkHere(const APlayerController* Controller, const FString& Name, FString& OutMessage);

	/**
	 * Teleports to a mark and runs an eased move. Pan/Pitch in degrees, Dolly
	 * in cm along the mark's forward (negative walks backwards), Seconds for
	 * the move itself; a short settle before and a hold after are added.
	 */
	bool StartShot(
		APlayerController* Controller,
		const FString& Name,
		float PanDegrees,
		float PitchDegrees,
		float DollyCm,
		float Seconds,
		FString& OutMessage);

	void StopShot();
	bool IsShotRunning() const { return bShotRunning; }
	TArray<FString> ListMarks() const;
	FString GetMarksFilePath() const;

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bShotRunning; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ULoop9TrailerRigSubsystem, STATGROUP_Tickables); }

private:
	bool LoadMark(const FString& Name, FVector& OutLocation, FRotator& OutRotation) const;
	static float EaseInOut(float T);

	TWeakObjectPtr<APlayerController> ShotController;
	bool bShotRunning = false;
	bool bInputIgnored = false;
	float Elapsed = 0.0f;
	float MoveSeconds = 5.0f;
	float PanDegrees = 0.0f;
	float PitchDegrees = 0.0f;
	float DollyCm = 0.0f;
	FRotator BaseRotation = FRotator::ZeroRotator;
	FVector DollyDirection = FVector::ForwardVector;
	FString ShotName;
};
