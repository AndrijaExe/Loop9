#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MoveAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UMoveAnomalyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMoveAnomalyComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnomalyProbability = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Anomaly")
	bool bIsAnomalyActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	bool bUseTeleport = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	bool bAnomalyTransformIsWorldSpace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	FVector AnomalyLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	FRotator AnomalyRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	bool bCaptureNormalTransformOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	FVector NormalLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	FRotator NormalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Anomaly")
	bool bEnsureOwnerIsMovable = true;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ActivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void DeactivateAnomaly();
};
