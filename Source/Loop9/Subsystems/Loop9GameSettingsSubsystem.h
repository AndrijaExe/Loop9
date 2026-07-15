#pragma once

#include "CoreMinimal.h"
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
 * Master volume works out of the box (application-level multiplier). Music and
 * SFX volumes additionally need USoundClass assets assigned to the game's
 * sounds; point MusicSoundClassPath / SFXSoundClassPath at them in
 * DefaultGame.ini once those assets exist.
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
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const { return SFXVolume; }

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

private:
	void ApplyMasterVolume() const;
	void ApplyGamma() const;
	void ApplySoundClassVolumes(UWorld* World);
	void HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues Values);
	void PersistSettings();

	UPROPERTY(Config)
	float MasterVolume = 1.0f;

	UPROPERTY(Config)
	float MusicVolume = 1.0f;

	UPROPERTY(Config)
	float SFXVolume = 1.0f;

	UPROPERTY(Config)
	float Gamma = 2.2f;

	UPROPERTY(Config)
	float MouseSensitivity = 1.0f;

	UPROPERTY(Config)
	bool bInvertYAxis = false;

	/** Optional: sound class used by music assets (set in DefaultGame.ini). */
	UPROPERTY(Config)
	FSoftObjectPath MusicSoundClassPath;

	/** Optional: sound class used by SFX assets (set in DefaultGame.ini). */
	UPROPERTY(Config)
	FSoftObjectPath SFXSoundClassPath;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> VolumeSoundMix;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> SFXSoundClass;

	FDelegateHandle PostWorldInitHandle;
};
