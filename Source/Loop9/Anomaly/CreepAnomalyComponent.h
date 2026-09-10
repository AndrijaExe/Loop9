#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "CreepAnomalyComponent.generated.h"

class AAnomalyMovePoint;

/**
 * 1.1 "Creep": an object that is slowly moving while the player is on the
 * floor. Not the Move anomaly, which is already displaced when the lift opens;
 * here the vase is where it belongs at first and has drifted a hand's width
 * by the time the player walks past it again, at about a centimetre a second.
 *
 * Put it on the object itself. The destination is either a tagged
 * AAnomalyMovePoint (drawn like Move) or CreepOffset from the normal spot.
 * The object stops when it gets there; it never comes back on its own.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UCreepAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UCreepAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Creep; }

	/** How fast it drifts, in cm per second. 1.0 is slow enough to miss and fast enough to matter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creep Anomaly", meta = (ClampMin = "0.05"))
	float CreepSpeedCmPerSecond = 1.0f;

	/**
	 * Where it ends up when no move point is used, relative to the object's
	 * own axes at its normal position (so +X is "forward" for the object).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creep Anomaly")
	FVector CreepOffset = FVector(0.0f, 60.0f, 0.0f);

	/** Optional extra turn applied over the whole drift, e.g. a vase slowly twisting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creep Anomaly")
	FRotator CreepRotation = FRotator::ZeroRotator;

	/**
	 * Only drift while the player is not looking at it. Off by default: the
	 * point of the anomaly is that it moves in plain sight, too slowly to see.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creep Anomaly")
	bool bPauseWhileObserved = false;

	/** Camera-forward dot needed to count as "looking at it" when bPauseWhileObserved is on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creep Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LookDotThreshold = 0.9f;

	/**
	 * Same contract as Move: explicit points win, otherwise the level is searched
	 * for AAnomalyMovePoint actors carrying CreepTargetTag. Empty tag = CreepOffset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creep Anomaly|Targets")
	TArray<TObjectPtr<AAnomalyMovePoint>> CreepTargetPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creep Anomaly|Targets")
	FName CreepTargetTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creep Anomaly")
	bool bEnsureOwnerIsMovable = true;

	/** How far it has drifted so far this floor, in cm. */
	UFUNCTION(BlueprintPure, Category = "Creep Anomaly")
	float GetDriftedDistance() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	bool IsObservedByPlayer() const;
	AAnomalyMovePoint* DrawTargetPoint() const;

	FVector NormalLocation = FVector::ZeroVector;
	FRotator NormalRotation = FRotator::ZeroRotator;
	FVector TargetLocation = FVector::ZeroVector;
	FRotator TargetRotation = FRotator::ZeroRotator;
	float TotalDistance = 0.0f;
	bool bDrifting = false;
};
