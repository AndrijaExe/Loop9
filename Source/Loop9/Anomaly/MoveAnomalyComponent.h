#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "MoveAnomalyComponent.generated.h"

class AAnomalyMovePoint;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UMoveAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UMoveAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Move; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	bool bUseTeleport = true;

	/**
	 * Points this object may be moved to, one drawn at random per activation.
	 * Leave empty and set MoveTargetTag to have the level searched for matching
	 * AAnomalyMovePoint actors instead, which keeps the Blueprint free of
	 * per-level references.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly|Targets")
	TArray<TObjectPtr<AAnomalyMovePoint>> MoveTargetPoints;

	/**
	 * Tag used when searching the level for points, and the only way to opt into
	 * that search. Only points carrying the same PointTag are eligible, so one
	 * object's hiding places never become another's.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly|Targets")
	FName MoveTargetTag = NAME_None;

	/**
	 * Avoids reusing the point from the previous activation while another one is
	 * available, so a repeated Move anomaly does not look like the same trick.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly|Targets")
	bool bAvoidRepeatingLastPoint = true;

	/**
	 * Legacy single destination, used only when no move points are found. It is
	 * ignored when left at zero: a world-space zero is the world origin, which
	 * is almost never a place the player can walk to, and shipping that as a
	 * silent default is what puts objects on a floor that does not exist.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	bool bAnomalyTransformIsWorldSpace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	FVector AnomalyLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	FRotator AnomalyRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	bool bCaptureNormalTransformOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	FVector NormalLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	FRotator NormalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	bool bEnsureOwnerIsMovable = true;

protected:
	virtual void BeginPlay() override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	/** Enabled, tag-matching points, from the explicit list or the level search. */
	TArray<AAnomalyMovePoint*> GatherEligiblePoints() const;

	/** Weighted draw that also honours bAvoidRepeatingLastPoint. */
	AAnomalyMovePoint* DrawTargetPoint(const TArray<AAnomalyMovePoint*>& Points) const;

	UPROPERTY(Transient)
	TWeakObjectPtr<AAnomalyMovePoint> LastUsedPoint;
};
