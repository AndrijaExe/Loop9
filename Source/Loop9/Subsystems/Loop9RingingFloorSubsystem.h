#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Loop9RingingFloorSubsystem.generated.h"

class AAI_Friend;
class ALiftDoorWing;
class ALoopElevatorTransitionDirector;
class ULightComponent;

/**
 * 1.1: the floor where a phone that is not Dragojlo's rings.
 *
 * Sequence, from the moment a ringing-phone anomaly activates on a desk
 * phone:
 *  1. The player steps out of the lit lift. Its doors close behind them and
 *     every light on the floor goes out except the lamp nearest the phone.
 *  2. They walk to it and pick up. One line is shown, read-only; some of the
 *     lines point at the wall that is not always a wall (the 1.1 ending).
 *  3. They close the chat. Only the lit lift opens again; the floor stays
 *     dark like every other 1.1 blackout, until the next floor or a reset.
 *
 * While the lift is held the lit button refuses presses. The dark lift is
 * never touched: a player who would rather not answer can still take the
 * wrong lift and start over. A MaxHoldSeconds safety net releases the lift
 * if nobody ever picks up.
 *
 * AAI_Friend and UAudioAnomalyComponent report into this; nothing here is
 * placed in the map.
 */
UCLASS()
class LOOP9_API ULoop9RingingFloorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	ULoop9RingingFloorSubsystem();

	/** A desk phone started ringing as this floor's anomaly. */
	void BeginRingingFloor(AAI_Friend* Phone);

	/** The ringing anomaly was reset (next floor, run reset, debug). Undoes everything. */
	void EndRingingFloor(const AAI_Friend* Phone);

	/** The player closed the chat after reading the line. Lift opens; lights stay dark until reset. */
	void OnRingingMessageRead(const AAI_Friend* Phone);

	/** True while the lit lift is closed and its button must refuse presses. */
	UFUNCTION(BlueprintPure, Category = "Loop9|Ringing Floor")
	bool IsLitLiftHeld() const { return bLitLiftHeld; }

	UFUNCTION(BlueprintPure, Category = "Loop9|Ringing Floor")
	bool IsRingingFloorActive() const { return RingingPhone.IsValid(); }

	/** How far from the arrival point counts as "out of the lift". */
	UPROPERTY(EditAnywhere, Category = "Loop9|Ringing Floor", meta = (ClampMin = "50.0"))
	float LiftExitDistanceCm = 230.0f;

	/** Search radius for the lamp that stays on above the phone. */
	UPROPERTY(EditAnywhere, Category = "Loop9|Ringing Floor", meta = (ClampMin = "50.0"))
	float PhoneLampSearchRadiusCm = 600.0f;

	/** Safety net: the lift reopens after this long even if nobody picks up. 0 = never. */
	UPROPERTY(EditAnywhere, Category = "Loop9|Ringing Floor", meta = (ClampMin = "0.0"))
	float MaxHoldSeconds = 240.0f;

	/** Played once, non-spatialized, the instant this anomaly turns the floor's lights off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop9|Ringing Floor|Audio")
	TObjectPtr<class USoundBase> BlackoutSound = nullptr;

	/** Trailer rig: skip BlackoutSound so the ring is the only thing heard when the floor drops. */
	void SetBlackoutSoundMuted(bool bMuted) { bBlackoutSoundMuted = bMuted; }

protected:
	virtual void Deinitialize() override;

private:
	void PollForLiftExit();
	void HoldLiftAndBlackout();
	/** Opens the lit lift only; used after the player answers, the floor stays dark. */
	void ReleaseLift();
	/** Opens the lift and also restores the lights; used on a full anomaly reset. */
	void ReleaseLiftAndRestore();
	void ClearState();
	ALoopElevatorTransitionDirector* FindDirector() const;

	TWeakObjectPtr<AAI_Friend> RingingPhone;
	TWeakObjectPtr<ALoopElevatorTransitionDirector> Director;
	bool bLitLiftHeld = false;
	bool bBlackoutApplied = false;
	bool bBlackoutSoundMuted = false;
	FTimerHandle ExitPollTimerHandle;
	FTimerHandle MaxHoldTimerHandle;
	double ExitPollStartedAtSeconds = 0.0;
};
