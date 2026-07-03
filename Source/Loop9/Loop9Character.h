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

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

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
	
public:
	ALoop9Character();

protected:
	virtual void BeginPlay() override;
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

	/** Called when player presses Interact key (E) */
	void OnInteract();

	/** Called when player presses Pause key (ESC) */
	void OnPause();

	/** Performs precise line trace from camera to find interactable objects */
	void PerformInteractTrace();

private:
	/** Pause menu instance */
	UPROPERTY()
	UUserWidget* PauseMenuInstance;

	float FootstepTimer = 0.0f;

	void TryPlayFootstep(float DeltaSeconds);

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

