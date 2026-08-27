// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Loop9Character.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class ALiftButton;
class AAI_Friend;
class ADoorInteractable;
class USoundBase;
class USpotLightComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdateSprintMeterDelegate, float, Percentage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSprintStateChangedDelegate, bool, bSprinting);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ALoop9Character : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Handheld light, aimed with the camera and off until the player asks for it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpotLightComponent* Flashlight;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* InteractAction;

	/** Pause Input Action (ESC key) */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* PauseAction;

	/**
	 * Flashlight toggle. Optional: when left empty the character builds its own
	 * action and binds F at runtime, so the light works in a fresh build before
	 * anyone opens the editor. Assign an IA asset here to put the key in
	 * IMC_Default alongside the others and make it reconfigurable.
	 */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* FlashlightAction;

	/**
	 * Hold to sprint. Optional: when empty the character builds its own
	 * action and binds Left Shift at runtime, same pattern as the flashlight.
	 */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* SprintAction;

	/** Sound played on each toggle, e.g. a plastic switch click. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Audio|Flashlight")
	TObjectPtr<USoundBase> FlashlightToggleSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Audio|Flashlight")
	float FlashlightToggleVolume = 0.6f;

	/** Maximum distance for interaction line trace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Interaction")
	float InteractMaxDistance = 300.0f;

	/** Debug mode - shows line trace visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Interaction")
	bool bShowInteractDebug = true;

	/** Pause Menu Widget Class (set in Blueprint) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category ="UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Footsteps")
	TArray<TObjectPtr<USoundBase>> FootstepSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Footsteps", meta = (ClampMin = "0.01"))
	float FootstepIntervalWalk = 0.48f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Footsteps", meta = (ClampMin = "0.01"))
	float FootstepIntervalRun = 0.33f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Footsteps", meta = (ClampMin = "0.0"))
	float MinFootstepSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Footsteps")
	float FootstepVolume = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Footsteps")
	float FootstepPitchMin = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Footsteps")
	float FootstepPitchMax = 1.05f;

	UPROPERTY(EditAnywhere, Category="Walk", meta = (ClampMin = "1.0", ClampMax = "2000.0", Units = "cm/s"))
	float WalkSpeed = 300.0f;

	UPROPERTY(EditAnywhere, Category="Sprint", meta = (ClampMin = "0.01", ClampMax = "1.0", Units = "s"))
	float SprintFixedTickTime = 0.03333f;

	UPROPERTY(EditAnywhere, Category="Sprint", meta = (ClampMin = "0.1", ClampMax = "30.0", Units = "s"))
	float SprintTime = 3.0f;

	UPROPERTY(EditAnywhere, Category="Sprint", meta = (ClampMin = "1.0", ClampMax = "2000.0", Units = "cm/s"))
	float SprintSpeed = 600.0f;

	UPROPERTY(EditAnywhere, Category="Recovery", meta = (ClampMin = "1.0", ClampMax = "2000.0", Units = "cm/s"))
	float RecoveringWalkSpeed = 200.0f;
	
public:
	ALoop9Character();

	UPROPERTY()
	FUpdateSprintMeterDelegate OnSprintMeterUpdated;

	UPROPERTY()
	FSprintStateChangedDelegate OnSprintStateChanged;

	float GetSprintMeterPercent() const
	{
		return SprintTime > 0.0f ? FMath::Clamp(SprintMeter / SprintTime, 0.0f, 1.0f) : 0.0f;
	}

	bool IsSprintActive() const { return bSprinting && !bRecovering; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	bool IsGameplayPresentationLocked() const;

	/** Called when player presses Interact key (E) */
	void OnInteract();

	/** Called when player presses Pause key (ESC) */
	void OnPause();

	/** Turns the handheld light on or off. */
	UFUNCTION(BlueprintCallable, Category="Flashlight")
	void ToggleFlashlight();

	UFUNCTION(BlueprintPure, Category="Flashlight")
	bool IsFlashlightOn() const;

	/** Performs precise line trace from camera to find interactable objects */
	void PerformInteractTrace();

private:
	/** Pause menu instance */
	UPROPERTY()
	UUserWidget* PauseMenuInstance;

	/** Runtime-built mapping context with gamepad keys for actions that only have keyboard bindings in IMC_Default. */
	UPROPERTY(Transient)
	TObjectPtr<class UInputMappingContext> GamepadFallbackContext;

	/** Stand-in action created only when no FlashlightAction asset was assigned. */
	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> RuntimeFlashlightAction;

	UPROPERTY(Transient)
	TObjectPtr<class UInputAction> RuntimeSprintAction;

	/** Returns the assigned action, creating the runtime stand-in on first use. */
	UInputAction* ResolveFlashlightAction();
	UInputAction* ResolveSprintAction();

	float FootstepTimer = 0.0f;
	float SprintMeter = 0.0f;
	bool bSprinting = false;
	bool bRecovering = false;
	FTimerHandle SprintTimer;

	void TryPlayFootstep(float DeltaSeconds);
	void DoStartSprint();
	void DoEndSprint();
	void SprintFixedTick();
	void EnsureSprintTimerRunning();
	void StopSprintTimerIfIdle();

	void RegisterGamepadFallbackContext();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	/** Adds gamepad keys for keyboard-only actions. Override to map additional actions in subclasses. */
	virtual void AddGamepadFallbackMappings(class UInputMappingContext* Context);
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

