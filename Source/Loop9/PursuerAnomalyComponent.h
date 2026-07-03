#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PursuerAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UPursuerAnomalyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPursuerAnomalyComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnomalyProbability = 0.4f;

	UPROPERTY(BlueprintReadOnly, Category = "Anomaly")
	bool bIsAnomalyActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer")
	TSubclassOf<class APursuerAnomalyCharacter> PursuerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer")
	bool bUseSpawnPoints = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer")
	FName SpawnPointTag = TEXT("PursuerSpawn");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer")
	TArray<TObjectPtr<class APursuerSpawnPoint>> ManualSpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer")
	bool bFallbackToRadiusSpawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer", meta = (ClampMin = "200.0"))
	float SpawnRadiusMin = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer", meta = (ClampMin = "200.0"))
	float SpawnRadiusMax = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer")
	float SpawnHeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer", meta = (ClampMin = "10.0"))
	float NavProjectionExtent = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer", meta = (ClampMin = "1"))
	int32 SpawnSearchMaxAttempts = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio")
	TObjectPtr<class USoundBase> ActiveAnomalyLoopSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio")
	TObjectPtr<class USoundAttenuation> ActiveAnomalyLoopAttenuation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio", meta = (ClampMin = "0.0"))
	float ActiveAnomalyLoopVolume = 0.7f;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void ActivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void DeactivateAnomaly();

private:
	UPROPERTY(Transient)
	TObjectPtr<class APursuerAnomalyCharacter> SpawnedPursuer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> ActiveAnomalyLoopAudioComponent = nullptr;

	UFUNCTION()
	void OnSpawnedPursuerDestroyed(AActor* DestroyedActor);

	bool TryGetSpawnTransformFromPoints(APawn* PlayerPawn, FTransform& OutTransform) const;
	bool TryGetSpawnTransformFromRadius(APawn* PlayerPawn, FTransform& OutTransform) const;
	bool ProjectToNavigation(const FVector& RawLocation, FVector& OutNavLocation) const;
};
