#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DoorLockStateAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UDoorLockStateAnomalyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoorLockStateAnomalyComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnomalyProbability = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Anomaly")
	bool bIsAnomalyActive = false;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ActivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void DeactivateAnomaly();

private:
	TObjectPtr<class ADoorInteractable> DoorRef = nullptr;
	bool bOriginalLockedState = false;
	bool bHasCapturedOriginal = false;
};
