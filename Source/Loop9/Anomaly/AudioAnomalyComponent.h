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

	/**
	 * 1.1: when this component sits on a desk phone (AAI_Friend), picking the
	 * phone up answers the ring. Off = the old behaviour (sound only).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Anomaly|Phone")
	bool bAnswerable = true;

	/** True while the manifestation is playing and has not been answered yet. */
	bool IsRinging() const;

	/** Stops the sound but keeps the anomaly active, so the floor still judges as wrong. */
	void Answer();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> RuntimeAudioComponent = nullptr;

	bool bAnswered = false;
};
