// Fill out your copyright notice in the Description page of Project Settings.

#include "LoopManagerSubsystem.h"
#include "TeleportPoint.h"
#include "AnomalyManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "AI_Friend.h"
#include "AI_ChatWidget.h"
#include "Blueprint/UserWidget.h"
#include "UI/EndingWidget.h"
#include "UI/ReplacementTerminalWidget.h"
#include "Loop9GameMode.h"
#include "Camera/PlayerCameraManager.h"

namespace
{
	struct FRelationshipStateView
	{
		float& Trust;
		float& Kindness;
		float& Cooperation;
		float& Suspicion;
		float& Dependency;
		float& AIStability;
	};

	struct FEndingEvaluationContext
	{
		float Trust = 0.0f;
		float Kindness = 0.0f;
		float Cooperation = 0.0f;
		float Suspicion = 0.0f;
		float Dependency = 0.0f;
		float AIStability = 0.0f;
		int32 TotalAdvances = 0;
		int32 TotalAIInteractions = 0;
	};

	constexpr bool bDebugAlwaysSpawnMoveAnomaly = false;
	constexpr float DefaultTrust = 0.5f;
	constexpr float DefaultKindness = 0.5f;
	constexpr float DefaultCooperation = 0.5f;
	constexpr float DefaultSuspicion = 0.2f;
	constexpr float DefaultDependency = 0.2f;
	constexpr float DefaultAIStability = 1.0f;

	FRelationshipStateView GetRelationshipStateView(ULoopManagerSubsystem& Subsystem)
	{
		return {
			Subsystem.Trust,
			Subsystem.Kindness,
			Subsystem.Cooperation,
			Subsystem.Suspicion,
			Subsystem.Dependency,
			Subsystem.AI_Stability
		};
	}

	void ClampRelationshipState(FRelationshipStateView State)
	{
		State.Trust = FMath::Clamp(State.Trust, 0.0f, 1.0f);
		State.Kindness = FMath::Clamp(State.Kindness, 0.0f, 1.0f);
		State.Cooperation = FMath::Clamp(State.Cooperation, 0.0f, 1.0f);
		State.Suspicion = FMath::Clamp(State.Suspicion, 0.0f, 1.0f);
		State.Dependency = FMath::Clamp(State.Dependency, 0.0f, 1.0f);
		State.AIStability = FMath::Clamp(State.AIStability, 0.0f, 1.0f);
	}

	void ResetRelationshipState(FRelationshipStateView State)
	{
		State.Trust = DefaultTrust;
		State.Kindness = DefaultKindness;
		State.Cooperation = DefaultCooperation;
		State.Suspicion = DefaultSuspicion;
		State.Dependency = DefaultDependency;
		State.AIStability = DefaultAIStability;
	}

	FEndingEvaluationContext BuildEndingContext(const ULoopManagerSubsystem& Subsystem)
	{
		return {
			Subsystem.Trust,
			Subsystem.Kindness,
			Subsystem.Cooperation,
			Subsystem.Suspicion,
			Subsystem.Dependency,
			Subsystem.AI_Stability,
			Subsystem.TotalAdvances,
			Subsystem.TotalAIInteractions
		};
	}

	ELoopEndingType EvaluateEndingType(const FEndingEvaluationContext& Context)
	{
		if (Context.Trust >= 0.75f
			&& Context.Kindness >= 0.70f
			&& Context.Cooperation >= 0.70f
			&& Context.Dependency >= 0.80f
			&& Context.AIStability <= 0.45f
			&& Context.TotalAIInteractions >= 10)
		{
			return ELoopEndingType::TheReplacement;
		}

		if (Context.Kindness >= 0.70f
			&& Context.Cooperation >= 0.70f
			&& Context.Trust >= 0.55f
			&& Context.AIStability <= 0.35f
			&& Context.TotalAdvances >= 6)
		{
			return ELoopEndingType::MergedMemory;
		}

		if (Context.Trust >= 0.60f
			&& Context.Kindness >= 0.65f
			&& Context.Cooperation >= 0.65f
			&& Context.Dependency <= 0.65f
			&& Context.AIStability >= 0.45f)
		{
			return ELoopEndingType::EscapeTogether;
		}

		if (Context.Trust >= 0.45f
			&& Context.Dependency >= 0.80f
			&& Context.Suspicion <= 0.35f
			&& Context.AIStability <= 0.80f)
		{
			return ELoopEndingType::ObedientFool;
		}

		if (Context.Trust >= 0.65f
			&& Context.Kindness <= 0.35f
			&& Context.Cooperation >= 0.50f
			&& Context.Dependency >= 0.55f
			&& Context.AIStability >= 0.50f)
		{
			return ELoopEndingType::ColdBetrayal;
		}

		if (Context.Suspicion >= 0.70f && Context.Trust <= 0.40f)
		{
			return ELoopEndingType::ParanoidSurvivor;
		}

		return ELoopEndingType::ObedientFool;
	}

	APlayerController* GetPrimaryPlayerController(UWorld* World)
	{
		return World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	}

	ACharacter* GetPrimaryPlayerCharacter(APlayerController* PlayerController)
	{
		return PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
	}

	ATeleportPoint* SelectRandomTeleportPoint(const TArray<TWeakObjectPtr<ATeleportPoint>>& TeleportPoints)
	{
		if (TeleportPoints.Num() == 0)
		{
			return nullptr;
		}

		const int32 RandomIndex = FMath::RandRange(0, TeleportPoints.Num() - 1);
		return TeleportPoints[RandomIndex].Get();
	}

	void ApplyTeleportFromPoint(APlayerController* PlayerController, ACharacter* PlayerCharacter, const ATeleportPoint* TeleportPoint)
	{
		if (!PlayerController || !PlayerCharacter || !TeleportPoint)
		{
			return;
		}

		const FVector TeleportLocation = TeleportPoint->GetTeleportLocation();
		const FRotator TeleportRotation = TeleportPoint->GetTeleportRotation();

		PlayerCharacter->SetActorLocation(TeleportLocation);
		PlayerCharacter->SetActorRotation(TeleportRotation);
		PlayerController->SetControlRotation(TeleportRotation);
	}

	void StartCameraFade(APlayerController* PlayerController, float FromAlpha, float ToAlpha, float Duration)
	{
		if (PlayerController && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, Duration, FLinearColor::Black, false, true);
		}
	}

	void ScheduleMainMenuReturn(UWorld* World, APlayerController* PlayerController)
	{
		if (!World)
		{
			return;
		}

		FTimerHandle ReturnTimer;
		World->GetTimerManager().SetTimer(ReturnTimer, [PlayerController, World]()
		{
			StartCameraFade(PlayerController, 0.0f, 1.0f, 1.0f);

			FTimerHandle OpenMenuTimer;
			World->GetTimerManager().SetTimer(OpenMenuTimer, [World]()
			{
				UGameplayStatics::OpenLevel(World, FName("MainMenu"));
			}, 1.0f, false);
		}, 2.5f, false);
	}

	UAnomalyManager* GetAnomalyManager(UGameInstance* GameInstance)
	{
		return GameInstance ? GameInstance->GetSubsystem<UAnomalyManager>() : nullptr;
	}

	int32 ComputeAnomalyTargetCount(int32 CurrentLoop, float AIStability)
	{
		const float LoopPressure = FMath::Clamp((CurrentLoop - 1) / 9.0f, 0.0f, 1.0f);
		const float StabilityPressure = FMath::Clamp(1.0f - AIStability, 0.0f, 1.0f);
		const float TargetCountFloat = 1.0f + (LoopPressure * 1.2f) + (StabilityPressure * 1.8f);
		return FMath::Clamp(FMath::RoundToInt(TargetCountFloat), 1, 4);
	}

	enum class ELoopAction : uint8
	{
		Advance,
		Reset
	};

	ELoopAction ResolveLoopAction(bool bAnomaliesExist, EButtonType ButtonType)
	{
		if (bAnomaliesExist)
		{
			return ButtonType == EButtonType::Increment ? ELoopAction::Reset : ELoopAction::Advance;
		}

		return ButtonType == EButtonType::Increment ? ELoopAction::Advance : ELoopAction::Reset;
	}

	void ApplyLoopTransitionPostActions(ULoopManagerSubsystem& Subsystem)
	{
		Subsystem.TeleportPlayerToExit();
		Subsystem.SetAnomalyDetected(false);
		Subsystem.GenerateAnomalyForNextLoop();
		Subsystem.ClearAllAIChats();
	}
}

void ULoopManagerSubsystem::OnElevatorButtonPressed(EButtonType ButtonType)
{
	const bool bAnomaliesExist = HasActiveAnomalies();
	const bool bWasCorrectDecision = (bAnomaliesExist && ButtonType == EButtonType::Reset)
		|| (!bAnomaliesExist && ButtonType == EButtonType::Increment);

	RegisterLoopDecision(bWasCorrectDecision, bAnomaliesExist, ButtonType);

	const ELoopAction Action = ResolveLoopAction(bAnomaliesExist, ButtonType);
	if (Action == ELoopAction::Advance)
	{
		AdvanceLoop();
	}
	else
	{
		ResetLoop();
	}
}

bool ULoopManagerSubsystem::HasActiveAnomalies() const
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	UAnomalyManager* AnomalyManager = GameInstance->GetSubsystem<UAnomalyManager>();
	if (!AnomalyManager)
	{
		return false;
	}

	int32 ActiveCount = AnomalyManager->GetActiveAnomalyCount();
	return ActiveCount > 0;
}

void ULoopManagerSubsystem::SetAnomalyDetected(bool bDetected)
{
	bAnomalyDetected = bDetected;
}

void ULoopManagerSubsystem::AdvanceLoop()
{
	if (bGameFinished)
	{
		return;
	}

	CurrentLoop++;
	TotalAdvances++;
	TickAIStabilityDecay();
	UE_LOG(LogTemp, Log, TEXT("Loop: %d"), CurrentLoop);

	if (CurrentLoop >= 10)
	{
		TriggerEndingSequence();
		return;
	}

	ApplyLoopTransitionPostActions(*this);
}

void ULoopManagerSubsystem::ResetLoop()
{
	if (bGameFinished)
	{
		return;
	}

	// RESET to beginning - Loop 1
	CurrentLoop = 1;
	TotalResets++;
	TickAIStabilityDecay();
	UE_LOG(LogTemp, Log, TEXT("Loop: %d"), CurrentLoop);

	ApplyLoopTransitionPostActions(*this);
}

void ULoopManagerSubsystem::RegisterAIInteraction()
{
	const FRelationshipStateView State = GetRelationshipStateView(*this);

	TotalAIInteractions++;
	State.Dependency += 0.03f;
	State.Cooperation += 0.01f;
	State.Trust += 0.005f;
	ClampStateValues();
}

void ULoopManagerSubsystem::ApplyAIDiagnosedSuspicionDelta(int32 Delta)
{
	const FRelationshipStateView State = GetRelationshipStateView(*this);
	const int32 ClampedDelta = FMath::Clamp(Delta, -1, 1);

	if (ClampedDelta > 0)
	{
		State.Suspicion += 0.05f;
		State.Trust -= 0.03f;
	}
	else if (ClampedDelta < 0)
	{
		State.Suspicion -= 0.04f;
		State.Trust += 0.02f;
	}

	ClampStateValues();
}

void ULoopManagerSubsystem::ApplyAIDiagnosedKindnessDelta(int32 Delta)
{
	const FRelationshipStateView State = GetRelationshipStateView(*this);
	const int32 ClampedDelta = FMath::Clamp(Delta, -1, 1);

	if (ClampedDelta > 0)
	{
		State.Kindness += 0.05f;
	}
	else if (ClampedDelta < 0)
	{
		State.Kindness -= 0.08f;
	}

	ClampStateValues();
}

void ULoopManagerSubsystem::ResetRunState()
{
	const FRelationshipStateView State = GetRelationshipStateView(*this);

	CurrentLoop = 1;
	bAnomalyDetected = false;
	bGameFinished = false;

	TotalResets = 0;
	TotalAdvances = 0;
	TotalAIInteractions = 0;

	ResetRelationshipState(State);

	ClampStateValues();
	ClearAllAIChats();
	UE_LOG(LogTemp, Log, TEXT("Loop: %d"), CurrentLoop);
}

void ULoopManagerSubsystem::RegisterPlayerMessage(const FString& Message)
{
	const FRelationshipStateView State = GetRelationshipStateView(*this);
	const FString Lower = Message.ToLower();

	// Kindness heuristics
	if (Lower.Contains(TEXT("hvala")) || Lower.Contains(TEXT("thank")) || Lower.Contains(TEXT("please"))
		|| Lower.Contains(TEXT("molim")) || Lower.Contains(TEXT("izvini")) || Lower.Contains(TEXT("sorry"))
		|| Lower.Contains(TEXT("bravo")) || Lower.Contains(TEXT("svaka cast")) || Lower.Contains(TEXT("dobar si"))
		|| Lower.Contains(TEXT("pomogao")) || Lower.Contains(TEXT("helped")) || Lower.Contains(TEXT("legendo")))
	{
		State.Kindness += 0.04f;
	}

	if (Lower.Contains(TEXT("idiot")) || Lower.Contains(TEXT("glup")) || Lower.Contains(TEXT("stupid"))
		|| Lower.Contains(TEXT("mrzim")) || Lower.Contains(TEXT("hate")) || Lower.Contains(TEXT("debil"))
		|| Lower.Contains(TEXT("retard")) || Lower.Contains(TEXT("kreten")) || Lower.Contains(TEXT("budalo"))
		|| Lower.Contains(TEXT("odjebi")) || Lower.Contains(TEXT("mars")))
	{
		State.Kindness -= 0.08f;
	}

	// Cooperation heuristics
	if (Lower.Contains(TEXT("vidim")) || Lower.Contains(TEXT("i see")) || Lower.Contains(TEXT("anomal"))
		|| Lower.Contains(TEXT("nema")) || Lower.Contains(TEXT("there is")) || Lower.Contains(TEXT("there isn't")))
	{
		State.Cooperation += 0.03f;
	}

	// Dependency heuristics
	if (Lower.Contains(TEXT("sta da radim")) || Lower.Contains(TEXT("what should i do"))
		|| Lower.Contains(TEXT("reci mi")) || Lower.Contains(TEXT("tell me")))
	{
		State.Dependency += 0.05f;
	}

	// Suspicion heuristics
	if (Lower.Contains(TEXT("laz")) || Lower.Contains(TEXT("laž")) || Lower.Contains(TEXT("ne verujem"))
		|| Lower.Contains(TEXT("don't trust")) || Lower.Contains(TEXT("sumnj")) || Lower.Contains(TEXT("sus")))
	{
		State.Suspicion += 0.04f;
		State.Trust -= 0.03f;
	}

	ClampStateValues();
}

void ULoopManagerSubsystem::RegisterLoopDecision(bool bWasCorrect, bool bAnomalyExisted, EButtonType ButtonType)
{
	const FRelationshipStateView State = GetRelationshipStateView(*this);

	if (bWasCorrect)
	{
		State.Trust += 0.04f;
		State.Suspicion -= 0.02f;
	}
	else
	{
		State.Trust -= 0.08f;
		State.Suspicion += 0.08f;
		State.AIStability -= 0.03f;
	}

	// If player follows anomaly logic correctly, cooperation rises a bit.
	if ((bAnomalyExisted && ButtonType == EButtonType::Reset)
		|| (!bAnomalyExisted && ButtonType == EButtonType::Increment))
	{
		State.Cooperation += 0.02f;
	}

	ClampStateValues();
}

void ULoopManagerSubsystem::TickAIStabilityDecay()
{
	const FRelationshipStateView State = GetRelationshipStateView(*this);

	// Slow systemic decay as loops go on; offset if relationship is healthier.
	State.AIStability -= 0.01f;
	State.AIStability += (State.Kindness > 0.7f ? 0.005f : 0.0f);
	State.AIStability += (State.Cooperation > 0.7f ? 0.005f : 0.0f);

	ClampStateValues();
}

void ULoopManagerSubsystem::ClampStateValues()
{
	const FRelationshipStateView State = GetRelationshipStateView(*this);
	ClampRelationshipState(State);

	UE_LOG(LogTemp, Log, TEXT("State -> Trust: %.2f | Kindness: %.2f | Cooperation: %.2f | Suspicion: %.2f | Dependency: %.2f | AI_Stability: %.2f"),
		Trust,
		Kindness,
		Cooperation,
		Suspicion,
		Dependency,
		AI_Stability);
}

ELoopEndingType ULoopManagerSubsystem::DetermineEndingType() const
{
	const FEndingEvaluationContext Context = BuildEndingContext(*this);
	return EvaluateEndingType(Context);
}

void ULoopManagerSubsystem::TriggerEndingSequence()
{
	if (bGameFinished)
	{
		return;
	}

	bGameFinished = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return;
	}

	// Lock player input
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	// Fade to black for 2 seconds
	StartCameraFade(PC, 0.0f, 1.0f, 2.0f);

	ELoopEndingType EndingType = DetermineEndingType();

	FTimerHandle EndingTimer;
	World->GetTimerManager().SetTimer(EndingTimer, [this, PC, EndingType]()
	{
		ShowEndingWidget(EndingType);
		PC->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
	}, 2.0f, false);
}

void ULoopManagerSubsystem::ShowEndingWidget(ELoopEndingType EndingType)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	TSubclassOf<UEndingWidget> WidgetClass = UEndingWidget::StaticClass();
	if (ALoop9GameMode* LoopGameMode = Cast<ALoop9GameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (LoopGameMode->EndingWidgetClass)
		{
			WidgetClass = LoopGameMode->EndingWidgetClass;
		}
	}

	UEndingWidget* EndingWidget = CreateWidget<UEndingWidget>(PC, WidgetClass);
	if (!EndingWidget)
	{
		return;
	}

	EndingWidget->InitializeEnding(EndingType, TotalResets, TotalAIInteractions);
	EndingWidget->AddToViewport(2000);

	if (EndingType == ELoopEndingType::TheReplacement)
	{
		FTimerHandle TerminalTimer;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(TerminalTimer, [this]()
			{
				ShowReplacementTerminal();
			}, 3.0f, false);
		}
		return;
	}

	// For all non-Replacement endings: fade to black, then return to MainMenu.
	ScheduleMainMenuReturn(GetWorld(), PC);
}

void ULoopManagerSubsystem::ShowReplacementTerminal()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
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
		return;
	}

	TerminalWidget->AddToViewport(3000);
	TerminalWidget->StartTerminalSequence();
}

void ULoopManagerSubsystem::FindTeleportPoints()
{
	EntryPoints.Empty();
	ExitPoints.Empty();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundPoints;
	UGameplayStatics::GetAllActorsOfClass(World, ATeleportPoint::StaticClass(), FoundPoints);

	for (AActor* Actor : FoundPoints)
	{
		ATeleportPoint* TeleportPoint = Cast<ATeleportPoint>(Actor);
		if (TeleportPoint)
		{
			if (TeleportPoint->TeleportType == ECustomTeleportType::Entry)
			{
				EntryPoints.Add(TeleportPoint);
			}
			else if (TeleportPoint->TeleportType == ECustomTeleportType::Exit)
			{
				ExitPoints.Add(TeleportPoint);
			}
		}
	}

}

void ULoopManagerSubsystem::TeleportPlayerToEntry()
{
	if (EntryPoints.Num() == 0)
	{
		FindTeleportPoints();
	}

	if (EntryPoints.Num() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController(World);
	ACharacter* PlayerCharacter = GetPrimaryPlayerCharacter(PlayerController);
	ATeleportPoint* SelectedEntry = SelectRandomTeleportPoint(EntryPoints);

	ApplyTeleportFromPoint(PlayerController, PlayerCharacter, SelectedEntry);
}

void ULoopManagerSubsystem::TeleportPlayerToExit()
{
	if (ExitPoints.Num() == 0)
	{
		FindTeleportPoints();
	}

	if (ExitPoints.Num() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = GetPrimaryPlayerController(World);
	ACharacter* PlayerCharacter = GetPrimaryPlayerCharacter(PlayerController);
	ATeleportPoint* SelectedExit = SelectRandomTeleportPoint(ExitPoints);

	ApplyTeleportFromPoint(PlayerController, PlayerCharacter, SelectedExit);
}

void ULoopManagerSubsystem::GenerateAnomalyForNextLoop()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UAnomalyManager* AnomalyManager = GetAnomalyManager(GameInstance);
	if (!AnomalyManager)
	{
		return;
	}

	// First, reset ALL anomalies (clear previous loop)
	AnomalyManager->ResetAllAnomalies();

 // Randomly decide if anomaly should exist (50% chance), unless debug override is enabled
	bool bShouldHaveAnomaly = bDebugAlwaysSpawnMoveAnomaly ? true : FMath::RandBool();

	if (bShouldHaveAnomaly)
	{
		const int32 AnomalyCount = bDebugAlwaysSpawnMoveAnomaly
			? 1
			: ComputeAnomalyTargetCount(CurrentLoop, AI_Stability);
		AnomalyManager->TriggerRandomAnomalies(AnomalyCount);

		int32 ActiveAfterTrigger = AnomalyManager->GetActiveAnomalyCount();
		if (ActiveAfterTrigger == 0)
		{
			AnomalyManager->ForceActivateAnyAnomaly();
			ActiveAfterTrigger = AnomalyManager->GetActiveAnomalyCount();
		}

		UE_LOG(LogTemp, Log, TEXT("Anomaly: %s"), ActiveAfterTrigger > 0 ? TEXT("YES") : TEXT("NO"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Anomaly: NO"));
	}
}

void ULoopManagerSubsystem::ClearAllAIChats()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundAIFriends;
	UGameplayStatics::GetAllActorsOfClass(World, AAI_Friend::StaticClass(), FoundAIFriends);

	for (AActor* Actor : FoundAIFriends)
	{
		if (const AAI_Friend* AIFriend = Cast<AAI_Friend>(Actor))
		{
			if (UAI_ChatWidget* AIChatWidget = Cast<UAI_ChatWidget>(AIFriend->GetChatWidgetInstance()))
			{
				AIChatWidget->ClearChat();
			}
		}
	}
}