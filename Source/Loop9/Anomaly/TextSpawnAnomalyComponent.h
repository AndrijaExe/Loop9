#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "TextSpawnAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UTextSpawnAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UTextSpawnAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Text; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Anomaly")
	FString MessageText = TEXT("You won't escape");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Anomaly")
	FLinearColor TextColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Anomaly")
	float WorldSize = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Anomaly")
	FVector RelativeOffset = FVector(0.0f, 0.0f, 40.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Anomaly")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text Anomaly")
	bool bFacePlayerOnActivate = true;

protected:
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<class UTextRenderComponent> SpawnedTextComponent = nullptr;
};
