// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Loop/LoopTypes.h"
#include "UI/EndingWidget.h"
#include "UI/ReplacementTerminalWidget.h"
#include "Loop9GameMode.generated.h"

class ULevelSequence;

UCLASS(abstract)
class LOOP9_API ALoop9GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALoop9GameMode();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UEndingWidget> EndingWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TMap<ELoopEndingType, TSubclassOf<UEndingWidget>> EndingWidgetClasses;

	/** Optional pre-widget cinematics. Missing entries use the existing fade fallback. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cinematics")
	TMap<ELoopEndingType, TSoftObjectPtr<ULevelSequence>> EndingSequences;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UReplacementTerminalWidget> ReplacementTerminalWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class ULoop9NotificationWidget> GameplayNotificationWidgetClass;

	/**
	 * Looping background music for the floor.
	 *
	 * Played from here as a 2D sound rather than left to an AAmbientSound placed
	 * in the map, for the same reason the menu does it in C++: a placed actor has
	 * to survive auto-activate, spatialization and a Looping flag on the wave,
	 * and any one of those failing leaves the level silent with nothing in the log.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> LevelMusicSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0"))
	float LevelMusicVolume = 0.35f;

	/** Live-apply the ambient slider to the music bed. */
	void ApplyAmbientVolume(float AmbientVolume);

	/** Silence the music bed while something else owns the mix (pursuer stinger). */
	void SetMusicSuppressed(bool bSuppressed);

	/** Start the bed if it is not running, for callers that know audio is now up. */
	void EnsureLevelMusicPlaying();

	bool IsLevelMusicPlaying() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StartLevelMusic();
	void StopLevelMusic();
	float ResolveMusicVolume() const;

	/**
	 * Restarts the bed when the wave turns out not to be flagged Looping. Cheaper
	 * than requiring every music asset to be authored correctly, and the failure
	 * it prevents is silence for the rest of the shift.
	 */
	UFUNCTION()
	void HandleLevelMusicFinished();

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> LevelMusicAudioComponent = nullptr;

	bool bLevelMusicSuppressed = false;
};
