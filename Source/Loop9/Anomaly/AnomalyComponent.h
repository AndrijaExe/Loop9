#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "AnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Hide; }

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ToggleAnomaly();

protected:
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;
	virtual void ResetToNormalState() override;
};
