#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TextSpawnAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UTextSpawnAnomalyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTextSpawnAnomalyComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnomalyProbability = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Anomaly")
	bool bIsAnomalyActive = false;

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

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ActivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void DeactivateAnomaly();

private:
	UPROPERTY(Transient)
	TObjectPtr<class UTextRenderComponent> SpawnedTextComponent = nullptr;
};
