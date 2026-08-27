#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "LoopNumberAnomalyComponent.generated.h"

/**
 * The loop counter on the floor is the anomaly: it flickers and turns into ?.
 * Place on ALoopNumberSign (the C++ actor already owns a native instance).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API ULoopNumberAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	ULoopNumberAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::LoopNumber; }

protected:
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	class ALoopNumberSign* FindLoopNumberSign() const;
};
