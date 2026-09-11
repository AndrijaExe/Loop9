// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Loop9BasePlayerController.h"
#include "Loop9PlayerController.generated.h"

class UBlinkOverlayWidget;
class UHorrorUI;
class ULoop9NotificationWidget;

UCLASS(abstract)
class LOOP9_API ALoop9PlayerController : public ALoop9BasePlayerController
{
	GENERATED_BODY()

public:
	ALoop9PlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI|Gameplay")
	TSubclassOf<ULoop9NotificationWidget> NotificationWidgetClass;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI|Cutscene")
	TSubclassOf<UBlinkOverlayWidget> BlinkOverlayWidgetClass;

	UPROPERTY()
	UBlinkOverlayWidget* BlinkOverlayInstance;

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual UUserWidget* GetInteractionPromptWidget() const override;

	UPROPERTY(EditAnywhere, Category="UI|Gameplay")
	TSubclassOf<UHorrorUI> GameplayUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UHorrorUI> GameplayUI;

public:
	UFUNCTION(BlueprintCallable, Category="UI|Cutscene")
	void CreateBlinkOverlay();

	UFUNCTION(BlueprintCallable, Category="UI|Cutscene")
	void PlayBlink(float Duration = 0.35f);

	UFUNCTION(BlueprintCallable, Category="UI|Cutscene")
	void RemoveBlinkOverlay();

	/**
	 * 1.1: a short full-screen "bad signal" burst (black frames, jumping scan
	 * bars) with an optional sound. Built in code, needs no widget class.
	 */
	UFUNCTION(BlueprintCallable, Category="UI|Cutscene")
	void PlaySignalBurst(float Duration = 0.6f);

	/** Played with the burst. Unset = PhoneLineCut from the phone folder when it exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI|Cutscene")
	TObjectPtr<class USoundBase> SignalBurstSound;

	// ---- Debug / testing console commands (tilde ~) ----
	/** List all registered anomalies. */
	UFUNCTION(Exec)
	void AnomalyList();

	/** Reset every anomaly to normal. */
	UFUNCTION(Exec)
	void AnomalyReset();

	/** Force any one inactive anomaly. */
	UFUNCTION(Exec)
	void AnomalyForceAny();

	/**
	 * Force an anomaly matching a filter.
	 * Examples:
	 *   AnomalyForce MaterialSwap
	 *   AnomalyForce Text
	 *   AnomalyForce OldMagazine
	 *   AnomalyForce I01
	 *   AnomalyForce I01 0
	 *   AnomalyForce Magazine 1
	 * Optional second arg = MaterialSwap variant index (0-based).
	 */
	UFUNCTION(Exec)
	void AnomalyForce(const FString& Args);

	/** Force every Light Flicker anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyFlicker();

	/** Force every Audio (phone) anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyPhone();

	/** Force the Pursuer anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyPursuer();

	/** Force every Hide anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyHide();

	/** Force every Move anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyMove();

	/** Force every DoorLock anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyDoor();

	/** Force every MaterialSwap anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyMaterial();

	/** Force every Text anomaly (material swaps and spawned notes). Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyText();

	/** Force every Scale anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyScale();

	/** Force every PhantomMessage anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyPhantom();

	/** Force the loop-counter glitch anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyLoopNumber();

	/** 1.1: force the back-turned Watcher figure. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyWatcher();

	/** 1.1: force every slow-drift Creep anomaly. Editor / non-Shipping only. */
	UFUNCTION(Exec)
	void AnomalyCreep();

	/**
	 * 1.1: undo the floor-wide line cut after answering a ringing phone, so the
	 * ring can be forced and answered again without changing floors.
	 */
	UFUNCTION(Exec)
	void PhoneLineRestore();

	/** Report every material-swap placement that would be invisible if it fired. */
	UFUNCTION(Exec)
	void AnomalyAuditMaterials();

	/** Print command help. */
	UFUNCTION(Exec)
	void AnomalyHelp();

	/**
	 * Report why the floor is or is not making sound: audio device, volumes, the
	 * music bed, and every ambient loop placed in the map. Cheaper than another
	 * round of guessing when the level goes silent.
	 */
	UFUNCTION(Exec)
	void AudioStatus();

	/**
	 * Debug: set loop 9 + relationship state for an ending, then press ADVANCE.
	 * Examples: EndingSetup EscapeTogether | EndingSetup 0 | EndingSetup Help
	 */
	UFUNCTION(Exec)
	void EndingSetup(const FString& Args);

	/** Print ending debug command help. */
	UFUNCTION(Exec)
	void EndingHelp();

	// ---- Trailer capture rig (ULoop9TrailerRigSubsystem); compiled out of Shipping ----

	/** Save the current spot and look direction under a name: TrailerMark clip2 */
	UFUNCTION(Exec)
	void TrailerMark(const FString& Name);

	/** Replay a mark with an eased move: TrailerShot clip2 pan 25 pitch -5 dolly 120 time 5 */
	UFUNCTION(Exec)
	void TrailerShot(const FString& Args);

	UFUNCTION(Exec)
	void TrailerStop();

	UFUNCTION(Exec)
	void TrailerList();

	/** Hide (0) or show (1) the gameplay HUD: crosshair, prompts, sprint meter. */
	UFUNCTION(Exec)
	void TrailerHUD(int32 Visible);

	UFUNCTION(Exec)
	void TrailerHelp();
};
