#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnomalyMovePoint.generated.h"

/**
 * An authored resting place for an object that a Move anomaly relocates.
 *
 * Drop two or three of these where you would accept finding the object, and the
 * Move anomaly picks one at random per activation. The point carries a rotation
 * as well as a position, so a phone can be left facing the wrong way rather than
 * simply standing somewhere else.
 *
 * Points exist because the alternative does not survive contact with a level.
 * A single hard-coded coordinate silently defaults to the world origin, which
 * can sit outside the building entirely, and a procedural offset has no idea
 * which floor it landed on. Both produce an object the player cannot reach,
 * which reads as a broken game rather than as an anomaly.
 */
UCLASS()
class LOOP9_API AAnomalyMovePoint : public AActor
{
	GENERATED_BODY()

public:
	AAnomalyMovePoint();

	/** Uncheck to retire a point without deleting it, e.g. while testing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly Move Point")
	bool bEnabled = true;

	/** Relative odds of being chosen against the other enabled points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly Move Point", meta = (ClampMin = "0.01"))
	float Weight = 1.0f;

	/**
	 * Restricts this point to Move anomalies whose MoveTargetTag matches, so one
	 * level can hold separate point sets for the phone, a chair and a mug.
	 * Leave None to accept any anomaly that finds it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly Move Point")
	FName PointTag = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anomaly Move Point")
	TObjectPtr<class USceneComponent> Root;

	/** Editor-only marker showing which way a moved object will face. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anomaly Move Point")
	TObjectPtr<class UArrowComponent> DirectionArrow;
};
