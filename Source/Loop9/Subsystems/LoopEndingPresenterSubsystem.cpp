#include "Subsystems/LoopEndingPresenterSubsystem.h"

#include "Subsystems/Loop9AchievementsSubsystem.h"
#include "Subsystems/Loop9TelemetrySubsystem.h"
#include "Subsystems/RelationshipSubsystem.h"
#include "Loop/LoopEndingEvaluator.h"
#include "Loop9GameMode.h"
#include "UI/EndingWidget.h"
#include "UI/ReplacementTerminalWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Containers/Ticker.h"
#include "Misc/QualifiedFrameTime.h"
#include "TimerManager.h"

namespace
{
	void StartCameraFade(APlayerController* PlayerController, float FromAlpha, float ToAlpha, float Duration)
	{
		if (PlayerController && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, Duration, FLinearColor::Black, false, true);
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

void ULoopEndingPresenterSubsystem::TriggerEndingSequence(URelationshipSubsystem* Relationship)
{
	if (PresentationState != EPresentationState::Idle)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !Relationship)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return;
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

	if (!TryPlayEndingSequence(EndingType))
	{
		PresentPendingEndingAfterFade(2.0f);
	}
}

bool ULoopEndingPresenterSubsystem::TryPlayEndingSequence(ELoopEndingType EndingType)
{
	UWorld* World = PresentationWorld.Get();
	ALoop9GameMode* LoopGameMode = Cast<ALoop9GameMode>(UGameplayStatics::GetGameMode(World));
	const TSoftObjectPtr<ULevelSequence>* SequenceReference =
		LoopGameMode ? LoopGameMode->EndingSequences.Find(EndingType) : nullptr;
	ULevelSequence* Sequence = SequenceReference ? SequenceReference->LoadSynchronous() : nullptr;
	if (!Sequence || !World)
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

void ULoopEndingPresenterSubsystem::HandleEndingSequenceFinished()
{
	if (!bEndingPresentationPending
		|| PresentationState != EPresentationState::PlayingSequence
		|| !ActiveSequencePlayer)
	{
		return;
	}

	if (EndingSequenceWatchdogTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(EndingSequenceWatchdogTickerHandle);
		EndingSequenceWatchdogTickerHandle.Reset();
	}

	UWorld* World = PresentationWorld.Get();
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!World || !PC)
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

	PresentationState = EPresentationState::FadingToWidget;
	StartCameraFade(PC, 0.0f, 1.0f, FadeDuration);

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
		FadeDuration,
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

	if (EndingType == ELoopEndingType::TheReplacement)
	{
		if (UWorld* World = GetWorld())
		{
			TWeakObjectPtr<ULoopEndingPresenterSubsystem> WeakThis(this);
			World->GetTimerManager().SetTimer(
				ReplacementTerminalTimerHandle,
				[WeakThis]()
				{
					if (WeakThis.IsValid()
						&& WeakThis->PresentationState == EPresentationState::ShowingWidget)
					{
						if (!WeakThis->ShowReplacementTerminal())
						{
							if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(
								WeakThis->PresentationWorld.Get(),
								0))
							{
								WeakThis->ReturnToMainMenu(PlayerController);
							}
						}
					}
				},
				3.0f,
				false);
		}
	}

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
	if (ActiveEndingWidget && ActiveEndingWidget->CurrentEndingType == ELoopEndingType::TheReplacement)
	{
		return;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		ReturnToMainMenu(PC);
	}
}

void ULoopEndingPresenterSubsystem::HandleReplacementTerminalContinueRequested()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		ReturnToMainMenu(PC);
	}
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
