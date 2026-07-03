#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "DoorLockStateAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UDoorLockStateAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UDoorLockStateAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::DoorLock; }

protected:
	virtual void BeginPlay() override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	TObjectPtr<class ADoorInteractable> DoorRef = nullptr;
	bool bOriginalLockedState = false;
	bool bHasCapturedOriginal = false;
};
