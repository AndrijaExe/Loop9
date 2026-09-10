#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoopNumberSign.generated.h"

class ULoopNumberAnomalyComponent;

UCLASS()
class LOOP9_API ALoopNumberSign : public AActor
{
	GENERATED_BODY()

public:
	ALoopNumberSign();

	/** Flicker and ? glitch only while the LoopNumber anomaly is active. */
	void SetAnomalyGlitchActive(bool bActive);
	bool IsAnomalyGlitchActive() const { return bAnomalyGlitchActive; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UTextRenderComponent* TextRender;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anomaly")
	TObjectPtr<ULoopNumberAnomalyComponent> LoopNumberAnomaly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign")
	FString Prefix = TEXT("LOOP");

	/**
	 * When > 0 the sign shows this number instead of the live loop. The TV in
	 * the apartment at the end of The Exit reads LOOP 1 while the run's real
	 * counter is still 4+.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign", meta = (ClampMin = "0"))
	int32 FixedLoopValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign")
	float UpdateInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign")
	FColor TextColor = FColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Visual")
	FColor AggressiveTextColor = FColor(255, 40, 40);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Visual", meta = (ClampMin = "2"))
	int32 MaxLoopForFullIntensity = 10;

	/**
	 * Cosmetic flicker without the anomaly. Interp so a Sequencer bool track can
	 * key it (the apartment TV at the end of The Exit); the flicker parameters
	 * below apply to both.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Loop Sign|Flicker")
	bool bEnableFlicker = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Flicker", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerStrength = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Flicker", meta = (ClampMin = "0.1"))
	float FlickerSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Flicker", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerDropoutChancePerSecond = 0.22f;

	/** Cosmetic "LOOP ?" glitch cycle without the anomaly; keyable like bEnableFlicker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Loop Sign|Glitch")
	bool bEnableGlitch = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Glitch", meta = (ClampMin = "1"))
	int32 GlitchEveryNLoops = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Glitch", meta = (ClampMin = "0.1"))
	float GlitchInterval = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Glitch", meta = (ClampMin = "0.05"))
	float GlitchDuration = 0.55f;

private:
	float UpdateAccumulator = 0.0f;
	int32 LastShownLoop = INDEX_NONE;
	float RunningTime = 0.0f;
	bool bFlickerDropout = false;
	float GlitchAccumulator = 0.0f;
	float GlitchElapsed = 0.0f;
	bool bGlitchActive = false;
	bool bAnomalyGlitchActive = false;
	FTimerHandle IdleRefreshTimerHandle;

	/** Anomaly glitch or either cosmetic flag: the sign needs its tick. */
	bool WantsPresentationFx() const { return bAnomalyGlitchActive || bEnableFlicker || bEnableGlitch; }
	void RefreshLoopText();
	void UpdateDynamicColor(int32 LoopValue);
	FString BuildGlitchText(int32 LoopValue) const;
	void ConfigurePresentationTick();
};
