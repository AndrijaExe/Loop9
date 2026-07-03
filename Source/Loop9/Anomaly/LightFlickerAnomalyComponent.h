#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "LightFlickerAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API ULightFlickerAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	ULightFlickerAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Light; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	FName TargetLightComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	float MinIntensityMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	float MaxIntensityMultiplier = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	float FlickerSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker")
	float RandomJitterStrength = 0.25f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	UPROPERTY()
	TArray<TObjectPtr<class ULightComponent>> TargetLights;

	TArray<float> BaseIntensities;
	float FlickerTime = 0.0f;
};
