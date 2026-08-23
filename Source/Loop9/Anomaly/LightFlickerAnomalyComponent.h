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

	/**
	 * Played at the light itself each time the flicker dips, so the player can
	 * hear which fitting is failing from another room and walk towards it.
	 * Defaults to the menu flicker sound; clear it to keep a light silent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker|Audio")
	TObjectPtr<class USoundBase> FlickerSound;

	/** Optional falloff. Leave empty to use the sound asset's own attenuation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker|Audio")
	TObjectPtr<class USoundAttenuation> FlickerSoundAttenuation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker|Audio", meta = (ClampMin = "0.0"))
	float FlickerSoundVolume = 0.7f;

	/**
	 * Floor on the dip that counts as a click worth hearing, as a fraction of
	 * the way from MinIntensityMultiplier to MaxIntensityMultiplier.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerSoundTriggerLevel = 0.3f;

	/**
	 * Shortest gap between two clicks. The intensity wave dips several times a
	 * second, and firing on every dip turns a failing fluorescent tube into a
	 * buzzsaw, so the sound is deliberately sparser than the light.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker|Audio", meta = (ClampMin = "0.05"))
	float MinSecondsBetweenFlickerSounds = 0.5f;

	/** Extra random delay on top of the minimum, so the rhythm is not metronomic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flicker|Audio", meta = (ClampMin = "0.0"))
	float FlickerSoundJitterSeconds = 0.9f;

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

	/** Seconds still owed before the next click may play. */
	float FlickerSoundCooldown = 0.0f;

	/** Suppresses repeat clicks while the light stays down in one long dip. */
	bool bWasBelowTriggerLevel = false;

	FVector GetFlickerSoundLocation() const;
	void TryPlayFlickerSound(float DeltaTime, float NormalizedIntensity);
};
