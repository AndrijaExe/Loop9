#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Loop/LoopTypes.h"
#include "LoopEndingSceneDirector.generated.h"

class ACharacter;
class ALiftDoorWing;
class APlayerController;
class UAudioComponent;
class UCameraComponent;
class ULightComponent;
class UPointLightComponent;
class USoundBase;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLoopEndingSceneFinished);

/**
 * C++ presentation layer for the six ending mini-scenes (camera, lights,
 * cosmetic props/audio). Mirrors ALoopElevatorTransitionDirector: Sequencer is
 * optional fallback only. Does not mutate loop/relationship/achievement state.
 */
UCLASS(Blueprintable)
class LOOP9_API ALoopEndingSceneDirector : public AActor
{
	GENERATED_BODY()

public:
	ALoopEndingSceneDirector();

	/** Starts a 5s mini-scene for the given ending. Returns false if already playing or missing controller. */
	UFUNCTION(BlueprintCallable, Category = "Ending Scene")
	bool PlayEnding(ELoopEndingType EndingType, APlayerController* InteractingController);

	/** Stops an in-progress scene or releases its held cinematic state without broadcasting. */
	UFUNCTION(BlueprintCallable, Category = "Ending Scene")
	void AbortScene();

	UFUNCTION(BlueprintPure, Category = "Ending Scene")
	bool IsPlaying() const { return bPlaying; }

	UFUNCTION(BlueprintPure, Category = "Ending Scene")
	float GetSceneDurationSeconds() const;

	UPROPERTY(BlueprintAssignable, Category = "Ending Scene")
	FLoopEndingSceneFinished OnSceneFinished;

	/** Dragojlo phone / AI friend actor — used as look / audio anchor. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Scene|Anchors")
	TObjectPtr<AActor> PhoneActor;

	/** Empty chair near Dragojlo's desk (Replacement / desk shots). */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Scene|Anchors")
	TObjectPtr<AActor> ChairActor;

	/** Preferred viewpoint after arriving in the lit elevator (Escape / Paranoid). */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Scene|Anchors")
	TObjectPtr<AActor> ArrivalViewpointActor;

	/** Optional office lights dimmed one-by-one in Obedient Fool (max 3 used). */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Scene|Anchors")
	TArray<TObjectPtr<AActor>> DimmableLightActors;

	/**
	 * Door wings for BOTH elevators (Cold Betrayal opens them all).
	 * If empty, every LiftDoorWing in the loaded world is used.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Scene|Cold Betrayal")
	TArray<TObjectPtr<ALiftDoorWing>> ColdBetrayalDoorWings;

	/**
	 * Optional placed Dragojlo / companion mesh shown beside the player in
	 * Escape Together. Hidden while idle. Prefer Companion Class when possible.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending Scene|Escape Together", meta = (DisplayName = "Companion Actor (optional)"))
	TObjectPtr<AActor> EscapeTogetherCompanionActor;

	/** Blueprint class to spawn beside the player (e.g. BP_PursuerAnomaly). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending Scene|Escape Together", meta = (DisplayName = "Companion Class", AllowAbstract = "false"))
	TSubclassOf<ACharacter> EscapeTogetherCompanionClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending Scene|Timing", meta = (ClampMin = "4.0", ClampMax = "6.0"))
	float SceneDurationSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending Scene|Audio")
	TObjectPtr<USoundBase> FootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending Scene|Audio")
	TObjectPtr<USoundBase> PhoneRingSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending Scene|Audio")
	TObjectPtr<USoundBase> LineCutSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending Scene|Audio")
	TObjectPtr<USoundBase> LightFlickerSound;

	/** Optional looping / ambient ring used only by Obedient Fool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending Scene|Audio")
	float ObedientPhoneRingVolume = 0.85f;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FSavedWorldLight
	{
		TWeakObjectPtr<ULightComponent> Light;
		float Intensity = 0.0f;
		bool bWasVisible = false;
	};

	struct FSavedLightIntensity
	{
		TWeakObjectPtr<ULightComponent> Light;
		float Intensity = 0.0f;
	};

	UPROPERTY(VisibleAnywhere, Category = "Ending Scene")
	TObjectPtr<UCameraComponent> SceneCamera;

	UPROPERTY(VisibleAnywhere, Category = "Ending Scene")
	TObjectPtr<UPointLightComponent> WarmKeyLight;

	UPROPERTY(VisibleAnywhere, Category = "Ending Scene")
	TObjectPtr<UPointLightComponent> AccentLight;

	/** Legacy second eye (BP instances may lack this — prefer runtime ColdEye*). */
	UPROPERTY(VisibleAnywhere, Category = "Ending Scene")
	TObjectPtr<UPointLightComponent> EyeLight;

	/** Runtime Cold Betrayal eyes (always created — avoids single-dot BP missing component). */
	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> ColdEyeLeft;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> ColdEyeRight;

	UPROPERTY(VisibleAnywhere, Category = "Ending Scene")
	TObjectPtr<UTextRenderComponent> MonitorText;

	TWeakObjectPtr<APlayerController> Controller;
	TWeakObjectPtr<AActor> PreviousViewTarget;
	TWeakObjectPtr<AActor> DuplicateProp;
	TWeakObjectPtr<AActor> ActiveEscapeCompanion;

	ELoopEndingType ActiveEnding = ELoopEndingType::ParanoidSurvivor;
	bool bPlaying = false;
	float ElapsedSeconds = 0.0f;

	FVector CamStartLocation = FVector::ZeroVector;
	FRotator CamStartRotation = FRotator::ZeroRotator;
	FVector CamEndLocation = FVector::ZeroVector;
	FRotator CamEndRotation = FRotator::ZeroRotator;

	FRotator ChairStartRotation = FRotator::ZeroRotator;
	FTransform EscapeCompanionStartTransform;
	bool bEscapeCompanionWasHidden = false;
	bool bEscapeCompanionCollisionEnabled = false;
	bool bEscapeCompanionTickEnabled = false;
	uint8 EscapeCompanionMovementMode = 0;
	uint8 EscapeCompanionCustomMovementMode = 0;
	bool bEscapeCompanionHasMovementSnapshot = false;
	bool bEscapeCompanionSpawned = false;

	TArray<FSavedLightIntensity> SavedLightIntensities;
	TArray<FSavedWorldLight> SavedWorldLights;
	TArray<TWeakObjectPtr<ALiftDoorWing>> ActiveColdDoors;
	uint8 FiredAudioMask = 0;
	uint8 FiredLightPulseMask = 0;
	bool bMonitorTextShown = false;
	bool bColdDoorsOpened = false;
	bool bColdLightsKilled = false;
	bool bColdEyesShown = false;
	bool bReplacementFadeStarted = false;
	bool bHoldingCinematicUntilAbort = false;
	float ColdDoorsOpenedAtSeconds = -1.0f;
	float ColdEyesShownAtSeconds = -1.0f;

	void BeginSceneSetup();
	void UpdateScene(float DeltaTime);
	void FinishScene(bool bBroadcastFinished);
	void RestoreWorldMods();

	float GetEffectiveSceneDuration() const;
	FVector ResolvePhoneLocation() const;
	FVector ResolveArrivalLocation() const;
	FVector ResolveChairLocation() const;
	void SetCameraPose(const FVector& Location, const FRotator& Rotation);
	void LerpCamera(float Alpha);
	void PlayOneShot(USoundBase* Sound, const FVector& Location, uint8 Bit);
	void ShowMonitorMessage(const FText& Message);
	void HideMonitorMessage();
	void ApplyLightDimIndex(int32 Index, bool bOff);
	void RestoreLightDims();
	void PulseAccent(float Intensity);
	void StartObedientPhoneRing();
	void StopObedientPhoneRing();
	void OrientMonitorTextToCamera();

	void BeginColdBetrayalDoors();
	void GatherColdBetrayalDoors();
	bool AreColdBetrayalDoorsOpen() const;
	void KillNearbyWorldLights();
	void RestoreNearbyWorldLights();
	void ShowColdBetrayalEyes();
	void HideColdBetrayalEyes();
	UPointLightComponent* EnsureRuntimeEyeLight(TObjectPtr<UPointLightComponent>& Slot, FName Name);
	void ConfigureColdEyeLight(UPointLightComponent* Light, const FVector& WorldLoc) const;

	void BeginEscapeTogetherCompanion();
	void CleanupEscapeTogetherCompanion();

	UFUNCTION()
	void HandleColdDoorMovementFinished(bool bIsOpen);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActivePhoneRingAudio;
};
