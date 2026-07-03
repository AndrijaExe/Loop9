#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Anomaly/AnomalyTypes.h"
#include "AnomalyComponentBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnomalyStateChanged, bool, bIsActive);

UCLASS(Abstract, BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UAnomalyComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UAnomalyComponentBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnomalyProbability = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Anomaly")
	bool bIsAnomalyActive = false;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	virtual void ActivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	virtual void DeactivateAnomaly();

	/** Ensures world state is normal when the anomaly is inactive (override for hide-style cleanup). */
	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	virtual void ResetToNormalState();

	virtual ELoopAnomalyType GetAnomalyType() const PURE_VIRTUAL(UAnomalyComponentBase::GetAnomalyType, return ELoopAnomalyType::Hide;);

	FName GetAnomalyTypeLabel() const { return GetLoopAnomalyTypeLabel(GetAnomalyType()); }

	UPROPERTY(BlueprintAssignable, Category = "Anomaly")
	FOnAnomalyStateChanged OnAnomalyActivated;

	UPROPERTY(BlueprintAssignable, Category = "Anomaly")
	FOnAnomalyStateChanged OnAnomalyDeactivated;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Returns true when anomaly state was applied successfully. */
	virtual bool ApplyAnomalyState();
	virtual void RestoreNormalState();
};
