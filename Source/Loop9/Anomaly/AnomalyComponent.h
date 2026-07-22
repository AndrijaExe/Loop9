#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "Components/PrimitiveComponent.h"
#include "AnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Hide; }

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ToggleAnomaly();

protected:
	virtual void BeginPlay() override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;
	virtual void ResetToNormalState() override;

private:
	struct FPrimitiveBaselineState
	{
		TWeakObjectPtr<UPrimitiveComponent> Component;
		bool bVisible = true;
		bool bHiddenInGame = false;
		ECollisionEnabled::Type CollisionEnabled = ECollisionEnabled::NoCollision;
	};

	void CaptureBaselineState();

	bool bBaselineCaptured = false;
	bool bOwnerHiddenInGame = false;
	bool bOwnerCollisionEnabled = true;
	FVector OwnerScale = FVector::OneVector;
	TArray<FPrimitiveBaselineState> PrimitiveBaselineStates;
};
