#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop/LoopTypes.h"
#include "LoopEndingPresenterSubsystem.generated.h"

class URelationshipSubsystem;
class UEndingWidget;
class UReplacementTerminalWidget;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class UWorld;

UCLASS()
class LOOP9_API ULoopEndingPresenterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void TriggerEndingSequence(URelationshipSubsystem* Relationship);
	void ReturnToMainMenu(APlayerController* PlayerController);

private:
	enum class EPresentationState : uint8
	{
		Idle,
		PlayingSequence,
		FadingToWidget,
		ShowingWidget,
		ReturningToMenu
	};

	bool TryPlayEndingSequence(ELoopEndingType EndingType);
	void PresentPendingEndingAfterFade(float FadeDuration);
	void ShowPendingEndingWidget();
	void CleanupActiveSequence(bool bStopPlayback);
	void ClearPresentationTimers();
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	bool HandleEndingSequenceWatchdog(float DeltaSeconds);

	UFUNCTION()
	void HandleEndingSequenceFinished();

	bool ShowEndingWidget(ELoopEndingType EndingType, int32 TotalResets, int32 TotalAIInteractions);
	bool ShowReplacementTerminal();
	TSubclassOf<UEndingWidget> ResolveEndingWidgetClass(ELoopEndingType EndingType) const;

	UFUNCTION()
	void HandleEndingContinueRequested();

	UFUNCTION()
	void HandleReplacementTerminalContinueRequested();

	UPROPERTY(Transient)
	TObjectPtr<UEndingWidget> ActiveEndingWidget;

	UPROPERTY(Transient)
	TObjectPtr<UReplacementTerminalWidget> ActiveTerminalWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ActiveSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> ActiveSequenceActor;

	ELoopEndingType PendingEndingType = ELoopEndingType::ParanoidSurvivor;
	int32 PendingTotalResets = 0;
	int32 PendingTotalAIInteractions = 0;
	bool bEndingPresentationPending = false;
	EPresentationState PresentationState = EPresentationState::Idle;
	TWeakObjectPtr<UWorld> PresentationWorld;

	FTimerHandle EndingPresentationTimerHandle;
	FTimerHandle SequenceCleanupAfterFadeTimerHandle;
	FTimerHandle ReplacementTerminalTimerHandle;
	FTimerHandle MainMenuTravelTimerHandle;
	FTSTicker::FDelegateHandle EndingSequenceWatchdogTickerHandle;
	FDelegateHandle WorldCleanupDelegateHandle;
};
