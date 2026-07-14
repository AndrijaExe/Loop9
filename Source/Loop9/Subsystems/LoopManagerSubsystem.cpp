#include "Subsystems/LoopManagerSubsystem.h"

#include "Subsystems/AnomalyManager.h"
#include "Subsystems/Loop9AchievementsSubsystem.h"
#include "Subsystems/RelationshipSubsystem.h"
#include "Subsystems/LoopEndingPresenterSubsystem.h"
#include "Loop/LoopEndingEvaluator.h"
#include "TeleportPoint.h"
#include "AI_Friend.h"
#include "AI_ChatWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

namespace
{
	constexpr bool bDebugAlwaysSpawnMoveAnomaly = false;

	URelationshipSubsystem* GetRelationshipSubsystem(UGameInstance* GameInstance)
	{
		return GameInstance ? GameInstance->GetSubsystem<URelationshipSubsystem>() : nullptr;
	}

	UAnomalyManager* GetAnomalyManager(UGameInstance* GameInstance)
	{
		return GameInstance ? GameInstance->GetSubsystem<UAnomalyManager>() : nullptr;
	}

	int32 ComputeAnomalyTargetCount(int32 LoopIndex, float AIStability)
	{
		const float LoopPressure = FMath::Clamp((LoopIndex - 1) / 9.0f, 0.0f, 1.0f);
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

	ATeleportPoint* SelectRandomTeleportPoint(const TArray<TWeakObjectPtr<ATeleportPoint>>& TeleportPoints)
	{
		if (TeleportPoints.Num() == 0)
		{
			return nullptr;
		}

		return TeleportPoints[FMath::RandRange(0, TeleportPoints.Num() - 1)].Get();
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
}

URelationshipSubsystem* ULoopManagerSubsystem::GetRelationship() const
{
	return GetRelationshipSubsystem(GetGameInstance());
}

int32 ULoopManagerSubsystem::GetTotalResets() const
{
	const URelationshipSubsystem* Relationship = GetRelationship();
	return Relationship ? Relationship->TotalResets : 0;
}

int32 ULoopManagerSubsystem::GetTotalAdvances() const
{
	const URelationshipSubsystem* Relationship = GetRelationship();
	return Relationship ? Relationship->TotalAdvances : 0;
}

int32 ULoopManagerSubsystem::GetTotalAIInteractions() const
{
	const URelationshipSubsystem* Relationship = GetRelationship();
	return Relationship ? Relationship->TotalAIInteractions : 0;
}

float ULoopManagerSubsystem::GetTrust() const
{
	const URelationshipSubsystem* Relationship = GetRelationship();
	return Relationship ? Relationship->Trust : 0.5f;
}

float ULoopManagerSubsystem::GetKindness() const
{
	const URelationshipSubsystem* Relationship = GetRelationship();
	return Relationship ? Relationship->Kindness : 0.5f;
}

float ULoopManagerSubsystem::GetCooperation() const
{
	const URelationshipSubsystem* Relationship = GetRelationship();
	return Relationship ? Relationship->Cooperation : 0.5f;
}

float ULoopManagerSubsystem::GetSuspicion() const
{
	const URelationshipSubsystem* Relationship = GetRelationship();
	return Relationship ? Relationship->Suspicion : 0.2f;
}

float ULoopManagerSubsystem::GetDependency() const
{
	const URelationshipSubsystem* Relationship = GetRelationship();
	return Relationship ? Relationship->Dependency : 0.2f;
}

float ULoopManagerSubsystem::GetAIStability() const
{
	const URelationshipSubsystem* Relationship = GetRelationship();
	return Relationship ? Relationship->AI_Stability : 1.0f;
}

void ULoopManagerSubsystem::OnElevatorButtonPressed(EButtonType ButtonType)
{
	const bool bAnomaliesExist = HasActiveAnomalies();
	const bool bWasCorrectDecision = (bAnomaliesExist && ButtonType == EButtonType::Reset)
		|| (!bAnomaliesExist && ButtonType == EButtonType::Increment);

	RegisterLoopDecision(bWasCorrectDecision, bAnomaliesExist, ButtonType);

	if (ULoop9AchievementsSubsystem* Achievements = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
	{
		FString AnomalyKey = TEXT("none");
		bool bRepeatAnomaly = false;
		if (UAnomalyManager* AnomalyManager = GetAnomalyManager(GetGameInstance()))
		{
			// Anomalies are still active here (reset happens below), so the
			// snapshot reflects what the player just judged.
			AnomalyManager->UpdateLoopAnomalyTracking(CurrentLoop);
			AnomalyKey = AnomalyManager->GetCurrentLoopAnomalyKey();
			bRepeatAnomaly = AnomalyManager->IsCurrentLoopAnomalyRepeat();
		}

		Achievements->NotifyLoopDecision(bWasCorrectDecision, bAnomaliesExist, AnomalyKey, bRepeatAnomaly);
	}

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
	const UAnomalyManager* AnomalyManager = GetAnomalyManager(GetGameInstance());
	return AnomalyManager && AnomalyManager->GetActiveAnomalyCount() > 0;
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
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->TotalAdvances++;
		Relationship->TickAIStabilityDecay();
	}

	if (ULoop9AchievementsSubsystem* Achievements = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
	{
		Achievements->NotifyLoopReached(CurrentLoop);
	}

	UE_LOG(LogTemp, Log, TEXT("Loop: %d"), CurrentLoop);

	if (CurrentLoop >= 10)
	{
		TriggerEndingSequence();
		return;
	}

	TeleportPlayerToExit();
	SetAnomalyDetected(false);
	GenerateAnomalyForNextLoop();
	ClearAllAIChats();
}

void ULoopManagerSubsystem::ResetLoop()
{
	if (bGameFinished)
	{
		return;
	}

	CurrentLoop = 1;
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->TotalResets++;
		Relationship->TickAIStabilityDecay();
	}

	UE_LOG(LogTemp, Log, TEXT("Loop: %d"), CurrentLoop);

	TeleportPlayerToExit();
	SetAnomalyDetected(false);
	GenerateAnomalyForNextLoop();
	ClearAllAIChats();
}

void ULoopManagerSubsystem::RegisterAIInteraction()
{
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->RegisterAIInteraction();

		if (ULoop9AchievementsSubsystem* Achievements = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
		{
			Achievements->NotifyAIMessageSent(Relationship->TotalAIInteractions);
		}
	}
}

void ULoopManagerSubsystem::ApplyAIDiagnosedSuspicionDelta(int32 Delta)
{
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->ApplyAIDiagnosedSuspicionDelta(Delta);
	}
}

void ULoopManagerSubsystem::ApplyAIDiagnosedKindnessDelta(int32 Delta)
{
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->ApplyAIDiagnosedKindnessDelta(Delta);
	}
}

void ULoopManagerSubsystem::ResetRunState()
{
	CurrentLoop = 1;
	bAnomalyDetected = false;
	bGameFinished = false;

	if (ULoop9AchievementsSubsystem* Achievements = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
	{
		Achievements->NotifyRunRestarted();
	}

	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->ResetRelationshipState();
	}

	ClearAllAIChats();
	UE_LOG(LogTemp, Log, TEXT("Loop: %d"), CurrentLoop);
}

void ULoopManagerSubsystem::RegisterPlayerMessage(const FString& Message)
{
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->RegisterPlayerMessage(Message);
	}
}

void ULoopManagerSubsystem::RegisterLoopDecision(bool bWasCorrect, bool bAnomalyExisted, EButtonType ButtonType)
{
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->RegisterLoopDecision(bWasCorrect, bAnomalyExisted, ButtonType);
	}
}

void ULoopManagerSubsystem::TickAIStabilityDecay()
{
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->TickAIStabilityDecay();
	}
}

ELoopEndingType ULoopManagerSubsystem::DetermineEndingType() const
{
	if (const URelationshipSubsystem* Relationship = GetRelationship())
	{
		return FLoopEndingEvaluator::Evaluate(Relationship->BuildEndingContext());
	}

	return ELoopEndingType::ParanoidSurvivor;
}

void ULoopManagerSubsystem::TriggerEndingSequence()
{
	if (bGameFinished)
	{
		return;
	}

	bGameFinished = true;

	if (ULoopEndingPresenterSubsystem* Presenter = GetGameInstance()->GetSubsystem<ULoopEndingPresenterSubsystem>())
	{
		Presenter->TriggerEndingSequence(GetRelationship());
	}
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
		if (ATeleportPoint* TeleportPoint = Cast<ATeleportPoint>(Actor))
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

	if (EntryPoints.Num() == 0 || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACharacter* PlayerCharacter = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
	ApplyTeleportFromPoint(PlayerController, PlayerCharacter, SelectRandomTeleportPoint(EntryPoints));
}

void ULoopManagerSubsystem::TeleportPlayerToExit()
{
	if (ExitPoints.Num() == 0)
	{
		FindTeleportPoints();
	}

	if (ExitPoints.Num() == 0 || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACharacter* PlayerCharacter = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
	ApplyTeleportFromPoint(PlayerController, PlayerCharacter, SelectRandomTeleportPoint(ExitPoints));
}

void ULoopManagerSubsystem::GenerateAnomalyForNextLoop()
{
	UAnomalyManager* AnomalyManager = GetAnomalyManager(GetGameInstance());
	if (!AnomalyManager)
	{
		return;
	}

	AnomalyManager->ResetAllAnomalies();

	const bool bShouldHaveAnomaly = bDebugAlwaysSpawnMoveAnomaly ? true : FMath::RandBool();
	if (!bShouldHaveAnomaly)
	{
		UE_LOG(LogTemp, Log, TEXT("Anomaly: NO"));
		return;
	}

	const int32 AnomalyCount = bDebugAlwaysSpawnMoveAnomaly
		? 1
		: ComputeAnomalyTargetCount(CurrentLoop, GetAIStability());

	AnomalyManager->TriggerRandomAnomalies(AnomalyCount);

	if (AnomalyManager->GetActiveAnomalyCount() == 0)
	{
		AnomalyManager->ForceActivateAnyAnomaly();
	}

	UE_LOG(LogTemp, Log, TEXT("Anomaly: %s"), AnomalyManager->GetActiveAnomalyCount() > 0 ? TEXT("YES") : TEXT("NO"));
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
