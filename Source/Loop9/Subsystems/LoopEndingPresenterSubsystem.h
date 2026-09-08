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
class ALoopEndingSceneDirector;
class APlayerController;
class UWorld;
struct FStreamableHandle;

UCLASS()
class LOOP9_API ULoopEndingPresenterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Returns true once presentation has accepted ownership and locked input. */
	bool TriggerEndingSequence(URelationshipSubsystem* Relationship);
	/**
	 * Same pipeline (archive, achievements, telemetry, memory, cutscene, card)
	 * for an ending that is triggered rather than scored — The Exit.
	 */
	bool TriggerForcedEnding(ELoopEndingType EndingType, URelationshipSubsystem* Relationship);
	void ReturnToMainMenu(APlayerController* PlayerController);

private:
	enum class EPresentationState : uint8
	{
		Idle,
		LoadingSequence,
		PlayingSequence,
		FadingToWidget,
		ShowingWidget,
		ReturningToMenu
	};

	bool BeginEndingPresentation(ELoopEndingType EndingType, URelationshipSubsystem* Relationship);
	bool TryPlayEndingSequence(ELoopEndingType EndingType);
	bool TryPlayEndingDirector(ELoopEndingType EndingType);
	bool StartLoadedEndingSequence(ULevelSequence* Sequence);
	void HandleEndingSequenceLoadComplete(uint32 LoadGeneration);
	void FinalizeEndingSequenceLoad(uint32 LoadGeneration, TWeakObjectPtr<UWorld> LoadWorld);
	void FallBackFromEndingSequenceLoad();
	void PresentPendingEndingAfterFade(float FadeDuration);
	void PresentPendingEndingFromBlack(float HoldBlackSeconds);
	void ShowPendingEndingWidget();
	void CleanupActiveSequence(bool bStopPlayback);
	void ClearPresentationTimers();
	void LockPresentationInput(APlayerController* PlayerController);
	void ReleasePresentationInputLocks();
	void AbortPresentationToIdle();
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

	UPROPERTY(Transient)
	TObjectPtr<ALoopEndingSceneDirector> ActiveSceneDirector;

	bool bPlayingDirectorScene = false;

	ELoopEndingType PendingEndingType = ELoopEndingType::ParanoidSurvivor;
	int32 PendingTotalResets = 0;
	int32 PendingTotalAIInteractions = 0;
	bool bEndingPresentationPending = false;
	bool bPresentationInputLocked = false;
	EPresentationState PresentationState = EPresentationState::Idle;
	TWeakObjectPtr<UWorld> PresentationWorld;
	TWeakObjectPtr<APlayerController> LockedPlayerController;
	TSoftObjectPtr<ULevelSequence> PendingSequenceReference;
	TSharedPtr<FStreamableHandle> ActiveSequenceLoadHandle;
	uint32 PresentationGeneration = 0;

	FTimerHandle EndingPresentationTimerHandle;
	FTimerHandle SequenceCleanupAfterFadeTimerHandle;
	FTimerHandle ReplacementTerminalTimerHandle;
	FTimerHandle MainMenuTravelTimerHandle;
	FTSTicker::FDelegateHandle EndingSequenceWatchdogTickerHandle;
	FDelegateHandle WorldCleanupDelegateHandle;
};
