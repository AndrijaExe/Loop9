#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "ScaleAnomalyComponent.generated.h"

/**
 * Subtle "spot the difference" anomaly: the owning actor becomes slightly
 * bigger or smaller than it was in the previous loop.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UScaleAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UScaleAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Scale; }

	/** Uniform multiplier applied to the normal scale while the anomaly is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale Anomaly", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ScaleMultiplier = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale Anomaly")
	bool bCaptureNormalScaleOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale Anomaly")
	FVector NormalScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale Anomaly")
	bool bEnsureOwnerIsMovable = true;

protected:
	virtual void BeginPlay() override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;
};
