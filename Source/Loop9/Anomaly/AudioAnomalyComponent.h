#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "AudioAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UAudioAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UAudioAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Audio; }

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

protected:
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> RuntimeAudioComponent = nullptr;
};
