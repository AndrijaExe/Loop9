#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "TimerManager.h"
#include "PursuerAnomalyComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UPursuerAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UPursuerAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Pursuer; }

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
	float ActiveAnomalyLoopVolume = 0.60f;

	/**
	 * Seconds after the pursuer exists before the tension bed replaces the
	 * floor music. Immediate music tells the player to take the lit elevator
	 * before they have looked around.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio", meta = (ClampMin = "0.0"))
	float TensionMusicDelaySeconds = 3.5f;

	/** 2D tension bed that replaces level ambience while the pursuer exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio")
	bool bReplaceLevelAmbience = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<class APursuerAnomalyCharacter> SpawnedPursuer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> ActiveAnomalyLoopAudioComponent = nullptr;

	bool bAmbienceSuppressed = false;

	UFUNCTION()
	void OnSpawnedPursuerDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void StartTensionMusic();
	void StopTensionMusic();
	void ClearTensionMusicDelay();

	UFUNCTION()
	void HandleTensionMusicFinished();

	FTimerHandle TensionMusicDelayHandle;

	bool TryGetSpawnTransformFromPoints(APawn* PlayerPawn, FTransform& OutTransform) const;
	bool TryGetSpawnTransformFromRadius(APawn* PlayerPawn, FTransform& OutTransform) const;
	bool ProjectToNavigation(const FVector& RawLocation, FVector& OutNavLocation) const;
	void CollectEnabledSpawnPoints(TArray<APursuerSpawnPoint*>& OutPoints) const;
};
