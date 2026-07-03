#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AudioAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UAudioAnomalyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAudioAnomalyComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnomalyProbability = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Anomaly")
	bool bIsAnomalyActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Anomaly")
	TObjectPtr<class USoundBase> AnomalySound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Anomaly")
	bool bLooping = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Anomaly")
	bool bPlaySoundAtLocation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Anomaly")
	bool bFollowOwnerWhenSpatial = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Anomaly")
	TObjectPtr<class USoundAttenuation> AttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Anomaly")
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Anomaly")
	float PitchMultiplier = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ActivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void DeactivateAnomaly();

private:
	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> RuntimeAudioComponent = nullptr;
};
