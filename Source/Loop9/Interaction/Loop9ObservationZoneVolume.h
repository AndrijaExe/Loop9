#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "Loop9ObservationZoneVolume.generated.h"

/**
 * Passive authored observation zone. It registers with the journal subsystem,
 * tracks player enter/exit, and has no authority over gameplay decisions.
 */
UCLASS(Blueprintable)
class LOOP9_API ALoop9ObservationZoneVolume : public ATriggerBox
{
	GENERATED_BODY()

public:
	ALoop9ObservationZoneVolume();

	/** Stable coarse location id, for example north_corridor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Observation Zone")
	FName ZoneId;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleComponentEntered(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleComponentExited(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);
};
