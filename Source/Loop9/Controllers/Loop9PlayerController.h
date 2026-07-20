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

	UFUNCTION(BlueprintCallable, Category="UI|Gameplay", meta=(DeprecatedFunction, DeprecationMessage="Use Gameplay Notifications subsystem Add Message instead"))
	void ShowGameplayNotification(const FText& Message, float DisplayDuration = 4.0f);

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

	/** Print command help. */
	UFUNCTION(Exec)
	void AnomalyHelp();
};
