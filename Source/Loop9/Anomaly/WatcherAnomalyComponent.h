#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "WatcherAnomalyComponent.generated.h"

/**
 * 1.1 "Watcher": a man standing with his back turned. He never moves and never
 * chases. He is gone the moment the player gets close, or the second time the
 * player looks at him after having looked away.
 *
 * Put this on an empty anchor actor placed where the figure should stand; the
 * figure spawns at the owner's transform, rotated so its back faces the player
 * at spawn time. FigureClass is any actor with a mesh (a Blueprint reusing the
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

	/** Coming this close makes him vanish. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "50.0"))
	float VanishDistance = 260.0f;

	/** Camera-forward dot needed to count as "looking at him". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LookDotThreshold = 0.86f;

	/**
	 * Out of sight for less than this is a doorframe or a chair crossing the
	 * trace, not a look-away; it neither arms the second look nor restarts the
	 * stare timer. 0 restores "any single poll out of sight counts".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "0.0"))
	float MinLookAwaySeconds = 0.5f;

	/** He also vanishes on the first look if it lasted this long; 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "0.0"))
	float MaxContinuousLookSeconds = 6.0f;

	/** Safety net: after this long on the floor he leaves on his own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher", meta = (ClampMin = "5.0"))
	float MaxLifetimeSeconds = 90.0f;

	/** Optional one-shot when he vanishes (a breath, a step). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher|Audio")
	TObjectPtr<class USoundBase> VanishSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watcher|Audio")
	TObjectPtr<class USoundAttenuation> VanishAttenuation = nullptr;

	/** True once the player has looked at him at least once this floor. */
	bool WasObserved() const { return bEverObserved; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	void Poll();
	bool IsObservedByPlayer(const APawn* PlayerPawn, const APlayerController* PlayerController) const;
	void Vanish(const TCHAR* Reason);
	void DestroyFigure();

	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedFigure = nullptr;

	FTimerHandle PollTimerHandle;
	double SpawnedAtSeconds = 0.0;
	/** Start of the current (flicker-tolerant) look; -1 while he is out of sight. */
	double LookStartedAtSeconds = -1.0;
	/** Last poll that saw him; decides whether a gap was a flicker or a look-away. */
	double LastSeenAtSeconds = -1.0;
	bool bEverObserved = false;
	bool bLookedAway = false;
};
