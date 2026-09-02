#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PursuerAnomalyCharacter.generated.h"

UCLASS()
class LOOP9_API APursuerAnomalyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APursuerAnomalyCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer", meta = (ClampMin = "0.0"))
	float CatchDistance = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer", meta = (ClampMin = "0.0"))
	float MoveAcceptanceRadius = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer", meta = (ClampMin = "0.05"))
	float MoveRefreshInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer", meta = (ClampMin = "0.0"))
	float MaxLifetime = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|VFX")
	TObjectPtr<class UNiagaraSystem> DespawnNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|VFX", meta = (ClampMin = "0.1"))
	float DespawnNiagaraMaxDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio")
	TObjectPtr<class USoundBase> DespawnSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio")
	TObjectPtr<class USoundAttenuation> DespawnSoundAttenuation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio")
	TObjectPtr<class USoundBase> MovingMurmurLoopSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio")
	TObjectPtr<class USoundAttenuation> MovingMurmurAttenuation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Audio", meta = (ClampMin = "0.0"))
	float MovingMurmurVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Perception")
	bool bOnlyChaseWhenNotObserved = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Perception", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float PlayerLookDotThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer|Perception", meta = (ClampMin = "0.01"))
	float ObservationCheckInterval = 0.08f;

private:
	float MoveRefreshAccumulator = 0.0f;
	float ObservationCheckAccumulator = 0.0f;
	float LifetimeElapsed = 0.0f;
	bool bHasCaughtPlayer = false;
	bool bIsObservedByPlayer = false;
	bool bWasObservedByPlayer = false;
	bool bMurmurPlaying = false;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> MovingMurmurAudioComponent = nullptr;

	TWeakObjectPtr<APawn> CachedPlayerPawn;
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	APawn* ResolvePlayerPawn();
	APlayerController* ResolvePlayerController();
	void UpdateChase();
	void CatchAndDespawn(bool bCaughtPlayer);
	bool ComputeIsObservedByPlayer(APawn* PlayerPawn, APlayerController* PlayerController) const;
	void ApplyObservationFreeze(bool bObservedNow);
	void UpdateMovingAudio();
};
