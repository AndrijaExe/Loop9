#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "MoveAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UMoveAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UMoveAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Move; }

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

protected:
	virtual void BeginPlay() override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;
};
