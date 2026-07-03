#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LightFlickerAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API ULightFlickerAnomalyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULightFlickerAnomalyComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnomalyProbability = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Anomaly")
	bool bIsAnomalyActive = false;

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

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ActivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void DeactivateAnomaly();

private:
    UPROPERTY()
	TArray<TObjectPtr<class ULightComponent>> TargetLights;

	TArray<float> BaseIntensities;
	float FlickerTime = 0.0f;
};
