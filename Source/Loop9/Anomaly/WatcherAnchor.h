#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WatcherAnchor.generated.h"

class UArrowComponent;
class UWatcherAnomalyComponent;

/**
 * 1.1: where the Watcher stands and which way he faces. Drop it in the level
 * like a PursuerSpawnPoint: the arrow is the direction he looks (his back is
 * toward the opposite side), the figure spawns at the anchor's transform.
 *
 * The WatcherAnomaly component is built in; set FigureClass, AnomalyZone and
 * the rest on it. A Blueprint child can set FigureClass once for all anchors.
 */
UCLASS(Blueprintable)
class LOOP9_API AWatcherAnchor : public AActor
{
	GENERATED_BODY()

public:
	AWatcherAnchor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Watcher")
	TObjectPtr<USceneComponent> Root;

	/** Editor-only arrow: the way he faces. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Watcher")
	TObjectPtr<UArrowComponent> FacingArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Watcher")
	TObjectPtr<UWatcherAnomalyComponent> Watcher;
};
