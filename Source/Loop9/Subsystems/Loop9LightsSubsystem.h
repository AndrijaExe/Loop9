#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Loop9LightsSubsystem.generated.h"

class ULightComponent;
class UAudioComponent;
class USoundBase;

/**
 * 1.1: one place that can put the floor in the dark and bring it back.
 *
 * Every light-touching path before this was per-actor (flicker) or a radius
 * around the ending scene. Two 1.1 beats need the whole office off at once —
 * the ringing phone leaves one lamp burning, walking into the Watcher cuts
 * everything for a few seconds — so the save/restore bookkeeping lives here,
 * where a loop change can also clear it.
 *
 * The player's own lights (flashlight on the pawn) and the lift-button
 * indicators are never touched: the first is the player's, the second says
 * which cabin is which even in the dark.
 */
UCLASS()
class LOOP9_API ULoop9LightsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	ULoop9LightsSubsystem();

	/**
	 * Turns off every world light except the ones listed. Calling it while a
	 * blackout is already on restores first, so the saved intensities are
	 * always the real ones and never a previous zero.
	 */
	UFUNCTION(BlueprintCallable, Category = "Loop9|Lights")
	void Blackout(const TArray<ULightComponent*>& KeepLit, bool bPlayWatcherAudio = false);

	/** Same, but the lights come back on their own after Seconds. 0 = until Restore(). */
	UFUNCTION(BlueprintCallable, Category = "Loop9|Lights")
	void BlackoutForSeconds(float Seconds, const TArray<ULightComponent*>& KeepLit, bool bPlayWatcherAudio = false);

	UFUNCTION(BlueprintCallable, Category = "Loop9|Lights")
	void Restore();

	UFUNCTION(BlueprintPure, Category = "Loop9|Lights")
	bool IsBlackedOut() const { return SavedLights.Num() > 0; }

	/**
	 * The world light closest to Location within MaxDistance, or null. Used to
	 * find "the lamp above the phone" without tagging anything in the map.
	 */
	UFUNCTION(BlueprintCallable, Category = "Loop9|Lights")
	ULightComponent* FindNearestLight(const FVector& Location, float MaxDistance) const;

	/** Played once, non-spatialized, the instant a Watcher blackout (bPlayWatcherAudio=true) turns lights off. Silent for other callers, e.g. the ringing-phone floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop9|Lights|Audio")
	TObjectPtr<USoundBase> BlackoutShatterSound = nullptr;

	/** Looped, non-spatialized, for as long as a Watcher blackout lasts; stopped on Restore() regardless of who caused it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop9|Lights|Audio")
	TObjectPtr<USoundBase> BlackoutAmbientSound = nullptr;

protected:
	virtual void Deinitialize() override;

private:
	struct FSavedLight
	{
		TWeakObjectPtr<ULightComponent> Light;
		float Intensity = 0.0f;
		bool bWasVisible = true;
	};

	static bool IsWorldLight(const ULightComponent* Light);

	TArray<FSavedLight> SavedLights;
	FTimerHandle RestoreTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BlackoutAmbientComponent = nullptr;
};
