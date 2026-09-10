// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
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

	/**
	 * 1.1 The Exit: the apartment map the ending travels to. On accept the
	 * office fades to black over TheExitFadeSeconds, opens this level, and
	 * ALoop9TheExitGameMode there plays its ExitSequence. Empty = legacy path:
	 * EndingSequences[TheExit] in place, else fade → card.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cinematics")
	TSoftObjectPtr<UWorld> TheExitLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cinematics", meta = (ClampMin = "0.0"))
	float TheExitFadeSeconds = 1.5f;

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

	/**
	 * Second floor bed. When set, the shift plays Level Music Sound once
	 * through, then this clip once through, then the first again. HorrorAmbience1
	 * is authored looping, so the swap is driven by wave duration, not
	 * OnAudioFinished.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> LevelMusicAltSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0"))
	float LevelMusicVolume = 0.30f;

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
	void PlayCurrentLevelMusicTrack();
	void AdvanceLevelMusicPlaylist();
	void ClearLevelMusicAdvanceTimer();
	float ResolveMusicVolume() const;
	float ResolveTrackDuration(class USoundBase* Sound) const;
	class USoundBase* ResolveCurrentLevelMusicTrack() const;

	/**
	 * Non-looping waves land here at the end of the file. Looping waves
	 * (HorrorAmbience1) never fire this, so the playlist timer is the authority.
	 */
	UFUNCTION()
	void HandleLevelMusicFinished();

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> LevelMusicAudioComponent = nullptr;

	FTimerHandle LevelMusicAdvanceTimerHandle;

	bool bLevelMusicSuppressed = false;
	bool bPlayingLevelMusicAlt = false;
};
