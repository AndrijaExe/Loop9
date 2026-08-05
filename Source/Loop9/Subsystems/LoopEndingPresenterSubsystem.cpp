#include "Subsystems/LoopEndingPresenterSubsystem.h"

#include "Interaction/LoopEndingSceneDirector.h"
#include "Loop/LoopEndingEvaluator.h"
#include "Loop9GameMode.h"
#include "Subsystems/Loop9AchievementsSubsystem.h"
#include "Subsystems/Loop9TelemetrySubsystem.h"
#include "UI/EndingWidget.h"
#include "UI/ReplacementTerminalWidget.h"
#include "Subsystems/RelationshipSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "TimerManager.h"
#include "Misc/QualifiedFrameTime.h"

namespace
{
	void StartCameraFade(APlayerController* PlayerController, float FromAlpha, float ToAlpha, float Duration)
	{
		if (PlayerController && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, Duration, FLinearColor::Black, false, true);
		}
	}

	void ClearCameraFade(APlayerController* PlayerController)
	{
		if (PlayerController && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StopCameraFade();
		}
	}
}

void ULoopEndingPresenterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldCleanupDelegateHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
		this,
		&ULoopEndingPresenterSubsystem::HandleWorldCleanup);
}

void ULoopEndingPresenterSubsystem::Deinitialize()
{
	++PresentationGeneration;
	if (WorldCleanupDelegateHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupDelegateHandle);
		WorldCleanupDelegateHandle.Reset();
	}

	ClearPresentationTimers();
	CleanupActiveSequence(true);
	bEndingPresentationPending = false;
	PresentationState = EPresentationState::Idle;
	PresentationWorld.Reset();

	Super::Deinitialize();
}

bool ULoopEndingPresenterSubsystem::TriggerEndingSequence(URelationshipSubsystem* Relationship)
{
	if (PresentationState != EPresentationState::Idle)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World || !Relationship)
	{
		return false;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return false;
	}

	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	PresentationState = EPresentationState::FadingToWidget;

	const ELoopEndingType EndingType = FLoopEndingEvaluator::Evaluate(Relationship->BuildEndingContext());
	const int32 TotalResets = Relationship->TotalResets;
	const int32 TotalAIInteractions = Relationship->TotalAIInteractions;

	if (ULoop9AchievementsSubsystem* AchievementsSubsystem = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
	{
		AchievementsSubsystem->NotifyRunFinished(EndingType, TotalResets, TotalAIInteractions);
	}

	if (ULoop9TelemetrySubsystem* Telemetry = GetGameInstance()->GetSubsystem<ULoop9TelemetrySubsystem>())
	{
		Telemetry->SendRunFinished(EndingType, TotalResets, TotalAIInteractions);
	}

	PendingEndingType = EndingType;
	PendingTotalResets = TotalResets;
	PendingTotalAIInteractions = TotalAIInteractions;
	bEndingPresentationPending = true;
	PresentationWorld = World;
	++PresentationGeneration;

	if (!TryPlayEndingSequence(EndingType))
	{
		if (EndingType == ELoopEndingType::ParanoidSurvivor)
		{
			// Glimpse already played; keep blackout briefly then open the ending card.
			PresentPendingEndingFromBlack(0.65f);
		}
		else
		{
			PresentPendingEndingAfterFade(2.0f);
		}
	}

	return true;
}

bool ULoopEndingPresenterSubsystem::TryPlayEndingSequence(ELoopEndingType EndingType)
{
	UWorld* World = PresentationWorld.Get();
	if (!World)
	{
		return false;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return false;
	}

	// Paranoid Survivor's look/glimpse already played while the elevator doors closed
	// (works for both lit and dark source cabins — uses whichever doors the pressed button owns).
	if (EndingType == ELoopEndingType::ParanoidSurvivor)
	{
		// Keep blackout; PresentPendingEndingFromBlack owns the hold → widget.
		return false;
	}

	if (TryPlayEndingDirector(EndingType))
	{
		return true;
	}

	ALoop9GameMode* LoopGameMode = Cast<ALoop9GameMode>(UGameplayStatics::GetGameMode(World));
	const TSoftObjectPtr<ULevelSequence>* SequenceReference =
		LoopGameMode ? LoopGameMode->EndingSequences.Find(EndingType) : nullptr;
	if (!SequenceReference || SequenceReference->IsNull())
	{
		return false;
	}

	PendingSequenceReference = *SequenceReference;
	PresentationState = EPresentationState::LoadingSequence;
	const uint32 LoadGeneration = PresentationGeneration;
	ActiveSequenceLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		PendingSequenceReference.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(
			this,
			&ULoopEndingPresenterSubsystem::HandleEndingSequenceLoadComplete,
			LoadGeneration));
	if (!ActiveSequenceLoadHandle.IsValid())
	{
		PendingSequenceReference.Reset();
		return false;
	}
	return true;
}

bool ULoopEndingPresenterSubsystem::TryPlayEndingDirector(ELoopEndingType EndingType)
{
	UWorld* World = PresentationWorld.Get();
	if (!World)
	{
		return false;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return false;
	}

	// Prefer C++ director (same model as elevator) when placed in the map.
	for (TActorIterator<ALoopEndingSceneDirector> It(World); It; ++It)
	{
		ALoopEndingSceneDirector* Director = *It;
		if (!Director || Director->IsPlaying())
		{
			continue;
		}

		if (!Director->PlayEnding(EndingType, PC))
		{
			continue;
		}

		ActiveSceneDirector = Director;
		bPlayingDirectorScene = true;
		Director->OnSceneFinished.AddDynamic(
			this,
			&ULoopEndingPresenterSubsystem::HandleEndingSequenceFinished);
		PresentationState = EPresentationState::PlayingSequence;

		const float WatchdogDelaySeconds = FMath::Max(Director->GetSceneDurationSeconds() + 5.0f, 5.0f);
		EndingSequenceWatchdogTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(
				this,
				&ULoopEndingPresenterSubsystem::HandleEndingSequenceWatchdog),
			WatchdogDelaySeconds);
		return true;
	}
	return false;
}

void ULoopEndingPresenterSubsystem::HandleEndingSequenceLoadComplete(uint32 LoadGeneration)
{
	const TWeakObjectPtr<ULoopEndingPresenterSubsystem> WeakThis(this);
	const TWeakObjectPtr<UWorld> LoadWorld(PresentationWorld.Get());
	if (UWorld* World = LoadWorld.Get())
	{
		// Always leave the streamable callback stack before creating playback or
		// taking the fallback path. Some already-resident assets complete inline.
		World->GetTimerManager().SetTimerForNextTick(
			[WeakThis, LoadWorld, LoadGeneration]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->FinalizeEndingSequenceLoad(LoadGeneration, LoadWorld);
				}
			});
	}
}

void ULoopEndingPresenterSubsystem::FinalizeEndingSequenceLoad(
	uint32 LoadGeneration,
	TWeakObjectPtr<UWorld> LoadWorld)
{
	if (LoadGeneration != PresentationGeneration
		|| !bEndingPresentationPending
		|| PresentationState != EPresentationState::LoadingSequence
		|| !LoadWorld.IsValid()
		|| PresentationWorld.Get() != LoadWorld.Get())
	{
		return;
	}

	ULevelSequence* Sequence = PendingSequenceReference.Get();
	ActiveSequenceLoadHandle.Reset();
	PendingSequenceReference.Reset();
	if (!StartLoadedEndingSequence(Sequence))
	{
		FallBackFromEndingSequenceLoad();
	}
}

bool ULoopEndingPresenterSubsystem::StartLoadedEndingSequence(ULevelSequence* Sequence)
{
	if (!Sequence)
	{
		return false;
	}

	UWorld* World = PresentationWorld.Get();
	if (!World)
	{
		return false;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	PlaybackSettings.bAutoPlay = false;

	ALevelSequenceActor* SpawnedActor = nullptr;
	ActiveSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		World,
		Sequence,
		PlaybackSettings,
		SpawnedActor);
	ActiveSequenceActor = SpawnedActor;
	if (!ActiveSequencePlayer)
	{
		CleanupActiveSequence(false);
		return false;
	}

	ActiveSequencePlayer->OnFinished.AddDynamic(
		this,
		&ULoopEndingPresenterSubsystem::HandleEndingSequenceFinished);
	ActiveSequencePlayer->OnStop.AddDynamic(
		this,
		&ULoopEndingPresenterSubsystem::HandleEndingSequenceFinished);
	PresentationState = EPresentationState::PlayingSequence;

	const double SequenceDurationSeconds = ActiveSequencePlayer->GetDuration().AsSeconds();
	const float WatchdogDelaySeconds = static_cast<float>(
		FMath::Max(SequenceDurationSeconds + 5.0, 5.0));
	EndingSequenceWatchdogTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(
			this,
			&ULoopEndingPresenterSubsystem::HandleEndingSequenceWatchdog),
		WatchdogDelaySeconds);

	ActiveSequencePlayer->Play();
	if (PresentationState == EPresentationState::PlayingSequence
		&& ActiveSequencePlayer
		&& !ActiveSequencePlayer->IsPlaying())
	{
		HandleEndingSequenceFinished();
	}
	return true;
}

void ULoopEndingPresenterSubsystem::FallBackFromEndingSequenceLoad()
{
	if (!bEndingPresentationPending
		|| PresentationState != EPresentationState::LoadingSequence)
	{
		return;
	}

	// A placed director may have become available while the asset was loading.
	if (TryPlayEndingDirector(PendingEndingType))
	{
		return;
	}

	if (PendingEndingType == ELoopEndingType::ParanoidSurvivor)
	{
		PresentPendingEndingFromBlack(0.65f);
	}
	else
	{
		PresentPendingEndingAfterFade(2.0f);
	}
}

void ULoopEndingPresenterSubsystem::HandleEndingSequenceFinished()
{
	if (!bEndingPresentationPending
		|| PresentationState != EPresentationState::PlayingSequence
		|| (!ActiveSequencePlayer && !bPlayingDirectorScene))
	{
		return;
	}

	if (EndingSequenceWatchdogTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(EndingSequenceWatchdogTickerHandle);
		EndingSequenceWatchdogTickerHandle.Reset();
	}

	UWorld* World = PresentationWorld.Get();
	if (!World)
	{
		CleanupActiveSequence(true);
		bEndingPresentationPending = false;
		PresentationState = EPresentationState::Idle;
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		CleanupActiveSequence(true);
		bEndingPresentationPending = false;
		PresentationState = EPresentationState::Idle;
		return;
	}

	PresentationState = EPresentationState::FadingToWidget;
	StartCameraFade(PC, 0.0f, 1.0f, 0.75f);
	TWeakObjectPtr<ULoopEndingPresenterSubsystem> WeakThis(this);
	World->GetTimerManager().SetTimer(
		SequenceCleanupAfterFadeTimerHandle,
		[WeakThis]()
		{
			if (!WeakThis.IsValid() || !WeakThis->bEndingPresentationPending)
			{
				return;
			}
			WeakThis->CleanupActiveSequence(true);
			WeakThis->ShowPendingEndingWidget();
		},
		0.75f,
		false);
}

bool ULoopEndingPresenterSubsystem::HandleEndingSequenceWatchdog(float /*DeltaSeconds*/)
{
	EndingSequenceWatchdogTickerHandle.Reset();
	HandleEndingSequenceFinished();
	return false;
}

void ULoopEndingPresenterSubsystem::PresentPendingEndingAfterFade(float FadeDuration)
{
	if (!bEndingPresentationPending)
	{
		return;
	}

	UWorld* World = PresentationWorld.Get();
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC || !World)
	{
		bEndingPresentationPending = false;
		PresentationState = EPresentationState::Idle;
		return;
	}

	// Coming from elevator blackout: clear held black, then optional soft fade into the widget.
	ClearCameraFade(PC);
	PresentationState = EPresentationState::FadingToWidget;
	if (FadeDuration > KINDA_SMALL_NUMBER)
	{
		StartCameraFade(PC, 0.0f, 1.0f, FadeDuration);
	}

	TWeakObjectPtr<ULoopEndingPresenterSubsystem> WeakThis(this);
	World->GetTimerManager().SetTimer(
		EndingPresentationTimerHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ShowPendingEndingWidget();
			}
		},
		FMath::Max(FadeDuration, 0.05f),
		false);
}

void ULoopEndingPresenterSubsystem::PresentPendingEndingFromBlack(float HoldBlackSeconds)
{
	if (!bEndingPresentationPending)
	{
		return;
	}

	UWorld* World = PresentationWorld.Get();
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC || !World)
	{
		bEndingPresentationPending = false;
		PresentationState = EPresentationState::Idle;
		return;
	}

	PresentationState = EPresentationState::FadingToWidget;
	StartCameraFade(PC, 1.0f, 1.0f, 0.0f);

	TWeakObjectPtr<ULoopEndingPresenterSubsystem> WeakThis(this);
	World->GetTimerManager().SetTimer(
		EndingPresentationTimerHandle,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ShowPendingEndingWidget();
			}
		},
		FMath::Max(HoldBlackSeconds, 0.05f),
		false);
}

void ULoopEndingPresenterSubsystem::ShowPendingEndingWidget()
{
	if (!bEndingPresentationPending)
	{
		return;
	}

	APlayerController* PlayerController =
		UGameplayStatics::GetPlayerController(PresentationWorld.Get(), 0);
	if (!PlayerController)
	{
		bEndingPresentationPending = false;
		PresentationState = EPresentationState::Idle;
		return;
	}

	ClearCameraFade(PlayerController);

	// Replacement goes straight into the green terminal — no summary card first.
	if (PendingEndingType == ELoopEndingType::TheReplacement)
	{
		if (!ShowReplacementTerminal())
		{
			bEndingPresentationPending = false;
			ReturnToMainMenu(PlayerController);
			return;
		}

		bEndingPresentationPending = false;
		PresentationState = EPresentationState::ShowingWidget;
		PlayerController->bShowMouseCursor = false;
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		return;
	}

	const bool bWidgetShown = ShowEndingWidget(
		PendingEndingType,
		PendingTotalResets,
		PendingTotalAIInteractions);
	if (!bWidgetShown)
	{
		bEndingPresentationPending = false;
		ReturnToMainMenu(PlayerController);
		return;
	}

	bEndingPresentationPending = false;
	PresentationState = EPresentationState::ShowingWidget;
	PlayerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	PlayerController->SetInputMode(InputMode);
}

void ULoopEndingPresenterSubsystem::CleanupActiveSequence(bool bStopPlayback)
{
	if (ActiveSequenceLoadHandle.IsValid())
	{
		ActiveSequenceLoadHandle->CancelHandle();
		ActiveSequenceLoadHandle.Reset();
	}
	PendingSequenceReference.Reset();

	if (IsValid(ActiveSceneDirector))
	{
		ActiveSceneDirector->OnSceneFinished.RemoveDynamic(
			this,
			&ULoopEndingPresenterSubsystem::HandleEndingSequenceFinished);
		if (bStopPlayback)
		{
			// A director stops ticking before it broadcasts completion, but
			// deliberately holds its camera/world state through our fade.
			ActiveSceneDirector->AbortScene();
		}
	}
	ActiveSceneDirector = nullptr;
	bPlayingDirectorScene = false;

	if (ActiveSequencePlayer)
	{
		ActiveSequencePlayer->OnFinished.RemoveDynamic(
			this,
			&ULoopEndingPresenterSubsystem::HandleEndingSequenceFinished);
		ActiveSequencePlayer->OnStop.RemoveDynamic(
			this,
			&ULoopEndingPresenterSubsystem::HandleEndingSequenceFinished);
		if (bStopPlayback && ActiveSequencePlayer->IsPlaying())
		{
			ActiveSequencePlayer->Stop();
		}
	}
	ActiveSequencePlayer = nullptr;

	if (IsValid(ActiveSequenceActor))
	{
		ActiveSequenceActor->Destroy();
	}
	ActiveSequenceActor = nullptr;
}

void ULoopEndingPresenterSubsystem::ClearPresentationTimers()
{
	if (EndingSequenceWatchdogTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(EndingSequenceWatchdogTickerHandle);
		EndingSequenceWatchdogTickerHandle.Reset();
	}

	if (UWorld* World = PresentationWorld.Get())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(EndingPresentationTimerHandle);
		TimerManager.ClearTimer(SequenceCleanupAfterFadeTimerHandle);
		TimerManager.ClearTimer(ReplacementTerminalTimerHandle);
		TimerManager.ClearTimer(MainMenuTravelTimerHandle);
	}
}

void ULoopEndingPresenterSubsystem::HandleWorldCleanup(
	UWorld* World,
	bool /*bSessionEnded*/,
	bool /*bCleanupResources*/)
{
	if (!World || PresentationWorld.Get() != World)
	{
		return;
	}

	++PresentationGeneration;
	ClearPresentationTimers();
	CleanupActiveSequence(false);
	ActiveEndingWidget = nullptr;
	ActiveTerminalWidget = nullptr;
	bEndingPresentationPending = false;
	PresentationState = EPresentationState::Idle;
	PresentationWorld.Reset();
}

TSubclassOf<UEndingWidget> ULoopEndingPresenterSubsystem::ResolveEndingWidgetClass(ELoopEndingType EndingType) const
{
	if (ALoop9GameMode* LoopGameMode = Cast<ALoop9GameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (const TSubclassOf<UEndingWidget>* MappedClass = LoopGameMode->EndingWidgetClasses.Find(EndingType))
		{
			if (*MappedClass)
			{
				return *MappedClass;
			}
		}

		if (LoopGameMode->EndingWidgetClass)
		{
			return LoopGameMode->EndingWidgetClass;
		}
	}

	return UEndingWidget::StaticClass();
}

bool ULoopEndingPresenterSubsystem::ShowEndingWidget(
	ELoopEndingType EndingType,
	int32 TotalResets,
	int32 TotalAIInteractions)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return false;
	}

	if (ActiveEndingWidget)
	{
		ActiveEndingWidget->RemoveFromParent();
		ActiveEndingWidget = nullptr;
	}

	const TSubclassOf<UEndingWidget> WidgetClass = ResolveEndingWidgetClass(EndingType);
	UEndingWidget* EndingWidget = CreateWidget<UEndingWidget>(PC, WidgetClass);
	if (!EndingWidget)
	{
		return false;
	}

	ActiveEndingWidget = EndingWidget;
	EndingWidget->InitializeEnding(EndingType, TotalResets, TotalAIInteractions);
	EndingWidget->OnContinueRequested.AddDynamic(this, &ULoopEndingPresenterSubsystem::HandleEndingContinueRequested);
	EndingWidget->AddToViewport(2000);

	return true;
}

bool ULoopEndingPresenterSubsystem::ShowReplacementTerminal()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return false;
	}

	TSubclassOf<UReplacementTerminalWidget> WidgetClass = UReplacementTerminalWidget::StaticClass();
	if (ALoop9GameMode* LoopGameMode = Cast<ALoop9GameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (LoopGameMode->ReplacementTerminalWidgetClass)
		{
			WidgetClass = LoopGameMode->ReplacementTerminalWidgetClass;
		}
	}

	UReplacementTerminalWidget* TerminalWidget = CreateWidget<UReplacementTerminalWidget>(PC, WidgetClass);
	if (!TerminalWidget)
	{
		return false;
	}

	ActiveTerminalWidget = TerminalWidget;
	TerminalWidget->OnContinueRequested.AddDynamic(this, &ULoopEndingPresenterSubsystem::HandleReplacementTerminalContinueRequested);
	TerminalWidget->AddToViewport(3000);
	TerminalWidget->StartTerminalSequence();
	return true;
}

void ULoopEndingPresenterSubsystem::HandleEndingContinueRequested()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		ReturnToMainMenu(PC);
	}
}

void ULoopEndingPresenterSubsystem::HandleReplacementTerminalContinueRequested()
{
	if (ActiveTerminalWidget)
	{
		ActiveTerminalWidget->RemoveFromParent();
		ActiveTerminalWidget = nullptr;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	// After the "You:_" beat + delay/sound, show the normal ending card with menu button.
	if (!ShowEndingWidget(ELoopEndingType::TheReplacement, PendingTotalResets, PendingTotalAIInteractions))
	{
		ReturnToMainMenu(PC);
		return;
	}

	PresentationState = EPresentationState::ShowingWidget;
	PC->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	PC->SetInputMode(InputMode);
}

void ULoopEndingPresenterSubsystem::ReturnToMainMenu(APlayerController* PlayerController)
{
	if (PresentationState == EPresentationState::ReturningToMenu)
	{
		return;
	}

	UWorld* World = PresentationWorld.IsValid() ? PresentationWorld.Get() : GetWorld();
	if (!World || !PlayerController)
	{
		return;
	}

	++PresentationGeneration;
	ClearPresentationTimers();
	CleanupActiveSequence(true);
	bEndingPresentationPending = false;
	PresentationState = EPresentationState::ReturningToMenu;

	if (ActiveEndingWidget)
	{
		ActiveEndingWidget->RemoveFromParent();
		ActiveEndingWidget = nullptr;
	}

	if (ActiveTerminalWidget)
	{
		ActiveTerminalWidget->RemoveFromParent();
		ActiveTerminalWidget = nullptr;
	}

	StartCameraFade(PlayerController, 1.0f, 1.0f, 0.0f);

	const TWeakObjectPtr<UWorld> WeakWorld(World);
	World->GetTimerManager().SetTimer(
		MainMenuTravelTimerHandle,
		[WeakWorld]()
		{
			if (WeakWorld.IsValid())
			{
				UGameplayStatics::OpenLevel(WeakWorld.Get(), FName("MainMenu"));
			}
		},
		0.1f,
		false);
}
