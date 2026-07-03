// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnomalyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnomalyStateChanged, bool, bIsActive);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOOP9_API UAnomalyComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAnomalyComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnomalyProbability;

	UPROPERTY(BlueprintReadOnly, Category = "Anomaly")
	bool bIsAnomalyActive;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ActivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void DeactivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ToggleAnomaly();

	UPROPERTY(BlueprintAssignable, Category = "Anomaly")
	FOnAnomalyStateChanged OnAnomalyActivated;

	UPROPERTY(BlueprintAssignable, Category = "Anomaly")
	FOnAnomalyStateChanged OnAnomalyDeactivated;
};
