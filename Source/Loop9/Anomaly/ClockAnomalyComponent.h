#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "ClockAnomalyComponent.generated.h"

UENUM(BlueprintType)
enum class EClockHandAxis : uint8
{
	Pitch,
	Yaw,
	Roll,
};

/**
 * Time-loop flavored anomaly: the clock's hands run backwards while active.
 *
 * Place on a clock actor whose hour/minute hands are separate scene components
 * (static meshes) and set their component names below. The hands rotate around
 * their local axis; pick the axis that matches how the hand meshes are built.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UClockAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UClockAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Clock; }

	/** Name of the minute-hand scene component on the owning actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clock Anomaly")
	FName MinuteHandComponentName = TEXT("MinuteHand");

	/** Name of the hour-hand scene component on the owning actor. Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clock Anomaly")
	FName HourHandComponentName = TEXT("HourHand");

	/** Local axis the hands rotate around. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clock Anomaly")
	EClockHandAxis RotationAxis = EClockHandAxis::Roll;

	/**
	 * Degrees per second for the minute hand while the anomaly is active.
	 * Negative = backwards. -30 deg/s is exaggerated enough to be noticed
	 * within a few seconds of looking at the clock.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clock Anomaly")
	float MinuteHandDegreesPerSecond = -30.0f;

	/** Degrees per second for the hour hand while the anomaly is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clock Anomaly")
	float HourHandDegreesPerSecond = -2.5f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	USceneComponent* ResolveHand(FName ComponentName) const;
	FRotator MakeHandRotation(float Degrees) const;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> MinuteHand;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> HourHand;

	FRotator MinuteHandNormalRotation = FRotator::ZeroRotator;
	FRotator HourHandNormalRotation = FRotator::ZeroRotator;
};
