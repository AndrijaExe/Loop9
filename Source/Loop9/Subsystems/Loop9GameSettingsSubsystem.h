#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "Loop9GameSettingsSubsystem.generated.h"

class USoundMix;
class USoundClass;

/**
 * Owns the game-specific settings that UGameUserSettings does not cover:
 * audio volumes, display gamma, mouse sensitivity and Y-axis inversion.
 *
 * Values persist in GameUserSettings.ini (Config properties + SaveConfig) and
 * are re-applied automatically on startup and on every world change.
 *
 * Master volume uses the audio device primary volume. Ambient volume needs a
 * USoundClass assigned to ambient loops; set AmbientSoundClassPath in
 * DefaultGame.ini.
 */
UCLASS(Config = GameUserSettings)
class LOOP9_API ULoop9GameSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Audio ---

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetAmbientVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetAmbientVolume() const { return AmbientVolume; }

	// --- Display ---

	/** Sets display gamma. 2.2 is neutral; sensible UI range is roughly 1.6 - 3.2. */
	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void SetGamma(float InGamma);

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	float GetGamma() const { return Gamma; }

	// --- Controls ---

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetMouseSensitivity(float Sensitivity);

	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	float GetMouseSensitivity() const { return MouseSensitivity; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetInvertYAxis(bool bInverted);

	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	bool IsYAxisInverted() const { return bInvertYAxis; }

	// --- Graphics preference (UI persistence) ---

	/** Preferred overall quality 0=Low..3=Epic. Stored separately because
	 *  UGameUserSettings::GetOverallScalabilityLevel() returns -1 when any
	 *  individual scalability setting diverges, which resets the combo to Low. */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPreferredGraphicsQuality(int32 QualityLevel);

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	int32 GetPreferredGraphicsQuality() const { return PreferredGraphicsQuality; }

	/** Immediately writes any live settings changes that are waiting on the debounce. */
	void FlushPendingSettings();

	/** Pause or resume level-placed AAmbientSound loops (pursuer tension stinger). */
	void SuppressLevelAmbience(bool bSuppress);

private:
	void ApplyMasterVolume() const;
	void ApplyGamma() const;
	void ApplySoundClassVolumes(UWorld* World);
	void ResolveAmbientSoundClass();
	UWorld* ResolveAudioWorld() const;
	void HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues Values);

	/**
	 * Re-runs the audio setup once the new world is actually running.
	 *
	 * OnPostWorldInitialization is too early to be the only chance: the world has
	 * no audio device yet, so PushSoundMixModifier quietly does nothing and every
	 * later call is skipped because the mix is recorded as pushed. That is why
	 * level music used to stay silent until the settings screen moved a slider
	 * and forced the override through by hand.
	 */
	bool HandleDeferredAudioBootstrap(float DeltaSeconds);

	/**
	 * Starts any level-placed ambient loop that failed to begin on its own.
	 * Volume is left alone here — the sound mix owns that — so the nudge cannot
	 * scale a track twice.
	 */
	void EnsureLevelAmbienceIsPlaying(UWorld* World) const;
	void ApplyLevelAmbienceSuppressState(UWorld* World) const;

	/** True when the loop is routed through the sound class the ambient slider drives. */
	bool IsMusicAmbience(const class UAudioComponent* Audio) const;

	/** The gameplay game mode, but only when it is playing the music bed itself. */
	class ALoop9GameMode* ResolveMusicOwningGameMode(UWorld* World) const;

	void SchedulePersistence();
	bool HandlePersistenceTicker(float DeltaSeconds);

	UPROPERTY(Config)
	float MasterVolume = 1.0f;

	UPROPERTY(Config)
	float AmbientVolume = 1.0f;

	UPROPERTY(Config)
	float Gamma = 2.2f;

	UPROPERTY(Config)
	float MouseSensitivity = 1.0f;

	UPROPERTY(Config)
	bool bInvertYAxis = false;

	UPROPERTY(Config)
	int32 PreferredGraphicsQuality = 2;

	/** Sound class used by ambient loops (set in DefaultGame.ini). */
	UPROPERTY(Config)
	FSoftObjectPath AmbientSoundClassPath;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> VolumeSoundMix;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> AmbientSoundClass;

	/** True after PushSoundMixModifier for the current world/audio context. */
	bool bVolumeMixPushed = false;

	TWeakObjectPtr<UWorld> VolumeMixWorld;

	FDelegateHandle PostWorldInitHandle;
	FTSTicker::FDelegateHandle PersistenceTickerHandle;
	FTSTicker::FDelegateHandle AudioBootstrapTickerHandle;
	bool bSettingsDirty = false;
	int32 LevelAmbienceSuppressCount = 0;
	int32 AudioBootstrapAttemptsLeft = 0;
};
