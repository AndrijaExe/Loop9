#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoopNumberSign.generated.h"

UCLASS()
class LOOP9_API ALoopNumberSign : public AActor
{
	GENERATED_BODY()

public:
	ALoopNumberSign();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UTextRenderComponent* TextRender;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign")
	FString Prefix = TEXT("LOOP");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign")
	float UpdateInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign")
	FColor TextColor = FColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Visual")
	FColor AggressiveTextColor = FColor(255, 40, 40);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Visual", meta = (ClampMin = "2"))
	int32 MaxLoopForFullIntensity = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Flicker")
	bool bEnableFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Flicker", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerStrength = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Flicker", meta = (ClampMin = "0.1"))
	float FlickerSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Flicker", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerDropoutChancePerSecond = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Glitch")
	bool bEnableGlitch = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Glitch", meta = (ClampMin = "1"))
	int32 GlitchEveryNLoops = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Glitch", meta = (ClampMin = "0.1"))
	float GlitchInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop Sign|Glitch", meta = (ClampMin = "0.05"))
	float GlitchDuration = 0.25f;

private:
	float UpdateAccumulator = 0.0f;
	int32 LastShownLoop = INDEX_NONE;
	float RunningTime = 0.0f;
	bool bFlickerDropout = false;
	float GlitchAccumulator = 0.0f;
	float GlitchElapsed = 0.0f;
	bool bGlitchActive = false;

	void RefreshLoopText();
  void UpdateDynamicColor(int32 LoopValue);
	FString BuildGlitchText(int32 LoopValue) const;
};
