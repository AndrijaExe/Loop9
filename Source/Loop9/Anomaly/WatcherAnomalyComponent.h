#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "WatcherAnomalyComponent.generated.h"

/**
 * 1.1 "Watcher": a man standing with his back turned. He never moves and never
 * chases.
 *
 * Two ways he leaves:
 *  - The player looks away. He is gone at once, quietly. When the player
 *    looks back at the empty spot, the lights on the floor go out.
 *  - The player walks up to him (any speed) or stares too long. The screen
 *    breaks into bad signal, the lights go out, and he is gone behind it.
 *
 * The lights stay out until the next floor, or until the anomaly is reset.
 *
 * Put this on an AWatcherAnchor (arrow shows which way he faces) or on any
 * actor; the figure spawns at the owner's transform, facing the owner's
 * forward. FigureClass is any actor with a mesh (a Blueprint reusing the
 * pursuer's skeletal mesh is the intended default). Author AnomalyZone /
 * AnomalyObjectKind on the component like every other anomaly.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UWatcherAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UWatcherAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::Watcher; }

	/** Actor to spawn as the figure. Must be set in the editor; nothing spawns without it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher")
	TSubclassOf<AActor> FigureClass;

	/** Coming this close makes him vanish, at any speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "50.0"))
	float VanishDistance = 260.0f;

	/** Camera-forward dot needed to count as "looking at him". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LookDotThreshold = 0.86f;

	/**
	 * Out of sight for less than this is a doorframe or a chair crossing the
	 * trace, not a look-away. 0 makes any single poll out of sight count.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "0.0"))
	float MinLookAwaySeconds = 0.5f;

	/** He also vanishes on the first look if it lasted this long (with the burst); 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "0.0"))
	float MaxContinuousLookSeconds = 6.0f;

	/** Safety net: after this long on the floor he leaves on his own, quietly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "5.0"))
	float MaxLifetimeSeconds = 90.0f;

	/** Optional one-shot when he vanishes (a breath, a step). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher|Audio")
	TObjectPtr<class USoundBase> VanishSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher|Audio")
	TObjectPtr<class USoundAttenuation> VanishAttenuation = nullptr;

	/** Played where he stood when the player looks back at the empty spot, as the lights go. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher|Audio")
	TObjectPtr<class USoundBase> LookBackSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher|Audio")
	TObjectPtr<class USoundAttenuation> LookBackAttenuation = nullptr;

	/** Length of the signal burst on approach / stare. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher|Vanish Effect", meta = (ClampMin = "0.15"))
	float BurstSeconds = 0.7f;

	/** Lights go out on approach, stare, and on the look back after a look-away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher|Vanish Effect")
	bool bBlackout = true;

	/** How long the floor stays dark. 0 = until the next floor or a reset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher|Vanish Effect", meta = (ClampMin = "0.0"))
	float BlackoutSeconds = 0.0f;

	/** True once the player has looked at him at least once this floor. */
	bool WasObserved() const { return bEverObserved; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	void Poll();
	bool IsLookingAt(const FVector& Target, const APawn* PlayerPawn, const APlayerController* PlayerController) const;
	/** Burst, lights out, achievement on approach, then gone. */
	void VanishWithBurst(const TCHAR* Reason, bool bApproached);
	/** Gone at once; the lights wait for the player to look back at the spot. */
	void VanishQuietly(const TCHAR* Reason);
	void Blackout();
	void DestroyFigure();
	void StopPolling();

	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedFigure = nullptr;

	FTimerHandle PollTimerHandle;
	double SpawnedAtSeconds = 0.0;
	/** Start of the current (flicker-tolerant) look; -1 while he is out of sight. */
	double LookStartedAtSeconds = -1.0;
	/** Last poll that saw him; decides whether a gap was a flicker or a look-away. */
	double LastSeenAtSeconds = -1.0;
	/** Chest height of where he stood, for the look back after a quiet vanish. */
	FVector LastFigureTarget = FVector::ZeroVector;
	bool bEverObserved = false;
	bool bAwaitingLookBack = false;
	bool bCausedBlackout = false;
};
