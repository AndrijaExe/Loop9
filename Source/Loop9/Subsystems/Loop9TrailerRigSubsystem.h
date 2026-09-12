#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Loop9TrailerRigSubsystem.generated.h"

class APlayerController;

/** One eased camera move. Steps without a Mark continue from wherever the previous step ended. */
struct FTrailerStep
{
	/** Teleport to this saved mark at the start of the step; empty = stay put. */
	FString Mark;
	float PanDegrees = 0.0f;
	float PitchDegrees = 0.0f;
	float DollyCm = 0.0f;
	float Seconds = 2.0f;
	/** AnomalyForce filter fired once during this step (Phone, Creep, Hide, Flicker, Watcher, Pursuer, Text, Move...). */
	FString ForceFilter;
	/** Where in the move (0..1) the force fires. */
	float ForceAtAlpha = 0.0f;
};

struct FTrailerScene
{
	FString Name;
	/** Which TrailerMark names the scene expects, for TrailerScenes. */
	FString RequiredMarks;
	FString Notes;
	TArray<FTrailerStep> Steps;
};

/**
 * Trailer capture rig: repeatable camera moves through the real player
 * camera, so every clip is genuine gameplay (the horror camera modifier's
 * lag, tremor and walk bob stay on) but nothing is done by hand.
 *
 * Walk to a spot, `TrailerMark desk`; later `TrailerShot desk pan 25 time 5`
 * teleports the pawn back there and pans 25 degrees over 5 s with an eased
 * curve, mouse and movement input ignored for the duration. `TrailerScene hook`
 * runs an authored multi-step sequence that can also force an anomaly at an
 * exact moment of a move. Marks persist in Saved/Trailer/Marks.ini.
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

	/** A single eased move from a mark; see FTrailerStep. */
	bool StartShot(
		APlayerController* Controller,
		const FString& MarkName,
		float PanDegrees,
		float PitchDegrees,
		float DollyCm,
		float Seconds,
		FString& OutMessage);

	/** Runs one of the authored scenes (see GetScenes). */
	bool StartScene(APlayerController* Controller, const FString& SceneName, FString& OutMessage);

	void StopShot();
	bool IsShotRunning() const { return bRunning; }
	TArray<FString> ListMarks() const;
	FString GetMarksFilePath() const;
	static const TArray<FTrailerScene>& GetScenes();

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bRunning; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ULoop9TrailerRigSubsystem, STATGROUP_Tickables); }

private:
	bool LoadMark(const FString& Name, FVector& OutLocation, FRotator& OutRotation) const;
	bool BeginSteps(APlayerController* Controller, const FString& Label, TArray<FTrailerStep>&& InSteps, FString& OutMessage);
	bool BeginStep(int32 Index);
	void FireForce(const FString& Filter);
	static float EaseInOut(float T);

	TWeakObjectPtr<APlayerController> Controller;
	TArray<FTrailerStep> Steps;
	int32 StepIndex = INDEX_NONE;
	FString Label;
	bool bRunning = false;
	bool bInputIgnored = false;
	bool bHudHidden = false;
	bool bMusicSuppressed = false;
	bool bForceFired = false;
	float StepElapsed = 0.0f;
	float StepSettle = 0.0f;
	FRotator BaseRotation = FRotator::ZeroRotator;
	FVector DollyDirection = FVector::ForwardVector;
};
