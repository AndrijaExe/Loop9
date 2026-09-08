#include "Subsystems/LoopManagerSubsystem.h"

#include "Subsystems/AnomalyManager.h"
#include "Subsystems/Loop9AchievementsSubsystem.h"
#include "Subsystems/Loop9ObservationJournalSubsystem.h"
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

	/**
	 * Chance that a floor past the baseline carries at least one anomaly.
	 * A clean floor is the exception that keeps the dark elevator worth taking,
	 * not a coin flip: at 50% half the run was spent confirming nothing, which
	 * reads as an empty level rather than as suspense. Floor 1 is exempt and
	 * always clean, because the opening phone call promises the player that.
	 */
	constexpr float AnomalyChancePerLoop = 0.8f;

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

void ULoopManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<ULoop9ObservationJournalSubsystem>();
	if (ULoop9ObservationJournalSubsystem* Journal =
		GetGameInstance()->GetSubsystem<ULoop9ObservationJournalSubsystem>())
	{
		Journal->OnZoneEntered.AddUObject(this, &ULoopManagerSubsystem::HandleObservationZoneEntered);
	}
}

void ULoopManagerSubsystem::Deinitialize()
{
	if (ULoop9ObservationJournalSubsystem* Journal =
		GetGameInstance()->GetSubsystem<ULoop9ObservationJournalSubsystem>())
	{
		Journal->OnZoneEntered.RemoveAll(this);
	}
	Super::Deinitialize();
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
	const FLoopElevatorDecision Decision = ResolveElevatorDecision(ButtonType);
	if (Decision.IsValid())
	{
		CommitElevatorDecision(Decision);
		FinishElevatorTransition(Decision.DecisionId);
	}
}

FLoopElevatorDecision ULoopManagerSubsystem::ResolveElevatorDecision(EButtonType ButtonType)
{
	FLoopElevatorDecision Decision;
	if (bGameFinished || bElevatorTransitionActive)
	{
		return Decision;
	}

	bElevatorTransitionActive = true;
	NextElevatorDecisionId = NextElevatorDecisionId >= MAX_int32 ? 1 : NextElevatorDecisionId + 1;
	PendingElevatorDecisionId = NextElevatorDecisionId;
	ActiveElevatorDecisionId = NextElevatorDecisionId;

	Decision.DecisionId = PendingElevatorDecisionId;
	Decision.ButtonType = ButtonType;
	Decision.bAnomaliesExisted = HasActiveAnomalies();
	Decision.bWasCorrect = (Decision.bAnomaliesExisted && ButtonType == EButtonType::Reset)
		|| (!Decision.bAnomaliesExisted && ButtonType == EButtonType::Increment);
	Decision.Action = ResolveLoopAction(Decision.bAnomaliesExisted, ButtonType);

	return Decision;
}

bool ULoopManagerSubsystem::CommitElevatorDecision(
	const FLoopElevatorDecision& Decision,
	bool bTeleportPlayer,
	bool bDeferEndingPresentation)
{
	if (!Decision.IsValid()
		|| bGameFinished
		|| !bElevatorTransitionActive
		|| ActiveElevatorDecisionId != Decision.DecisionId
		|| PendingElevatorDecisionId != Decision.DecisionId)
	{
		return false;
	}

	RegisterLoopDecision(Decision.bWasCorrect, Decision.bAnomaliesExisted, Decision.ButtonType);
	if (ULoop9ObservationJournalSubsystem* Journal =
		GetGameInstance()->GetSubsystem<ULoop9ObservationJournalSubsystem>())
	{
		Journal->RecordElevatorDecision(
			Decision.ButtonType == EButtonType::Reset ? FName(TEXT("lit")) : FName(TEXT("dark")),
			Decision.bWasCorrect);
	}

	DragojloCommitmentTracker.ApplyDecision(Decision.ButtonType);

	if (ULoop9AchievementsSubsystem* Achievements = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
	{
		FString AnomalyKey = TEXT("none");
		bool bRepeatAnomaly = false;
		if (UAnomalyManager* AnomalyManager = GetAnomalyManager(GetGameInstance()))
		{
			// Capture the judged floor before the world-state commit resets it.
			AnomalyManager->UpdateLoopAnomalyTracking(CurrentLoop);
			AnomalyKey = AnomalyManager->GetCurrentLoopAnomalyKey();
			bRepeatAnomaly = AnomalyManager->IsCurrentLoopAnomalyRepeat();
		}

		Achievements->NotifyLoopDecision(
			Decision.bWasCorrect,
			Decision.bAnomaliesExisted,
			AnomalyKey,
			bRepeatAnomaly);
	}

	PendingElevatorDecisionId = 0;
	if (Decision.Action == ELoopAction::Advance)
	{
		AdvanceLoopInternal(bTeleportPlayer, bDeferEndingPresentation);
	}
	else
	{
		ResetLoopInternal(bTeleportPlayer);
	}

	return true;
}

bool ULoopManagerSubsystem::FinishElevatorTransition(int32 DecisionId)
{
	if (!bElevatorTransitionActive || ActiveElevatorDecisionId != DecisionId)
	{
		return false;
	}

	PendingElevatorDecisionId = 0;
	ActiveElevatorDecisionId = 0;
	bElevatorTransitionActive = false;
	return true;
}

bool ULoopManagerSubsystem::CancelElevatorDecision(int32 DecisionId)
{
	if (!bElevatorTransitionActive
		|| ActiveElevatorDecisionId != DecisionId
		|| PendingElevatorDecisionId != DecisionId)
	{
		return false;
	}

	return FinishElevatorTransition(DecisionId);
}

void ULoopManagerSubsystem::CancelDeferredEnding()
{
	bDeferredEndingPresentation = false;
}

void ULoopManagerSubsystem::CompleteDeferredEnding()
{
	if (!bDeferredEndingPresentation)
	{
		return;
	}

	bDeferredEndingPresentation = false;
	TriggerEndingSequence();
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
	if (bElevatorTransitionActive)
	{
		return;
	}
	AdvanceLoopInternal(true, false);
}

void ULoopManagerSubsystem::AdvanceLoopInternal(bool bTeleportPlayer, bool bDeferEndingPresentation)
{
	if (bGameFinished)
	{
		return;
	}

	CurrentLoop++;
	NotifyAIFriendsLoopChanged();
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->NotifyLoopLeft();
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
		if (bDeferEndingPresentation)
		{
			bDeferredEndingPresentation = true;
		}
		else
		{
			TriggerEndingSequence();
		}
		return;
	}

	if (bTeleportPlayer)
	{
		TeleportPlayerToExit();
	}
	SetAnomalyDetected(false);
	GenerateAnomalyForNextLoop();
	ClearAllAIChats();
}

void ULoopManagerSubsystem::ResetLoop()
{
	if (bElevatorTransitionActive)
	{
		return;
	}
	ResetLoopInternal(true);
}

void ULoopManagerSubsystem::ResetLoopInternal(bool bTeleportPlayer)
{
	if (bGameFinished)
	{
		return;
	}

	CurrentLoop = 1;
	NotifyAIFriendsLoopChanged();
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->NotifyLoopLeft();
		Relationship->TotalResets++;
		Relationship->TickAIStabilityDecay();
	}

	UE_LOG(LogTemp, Log, TEXT("Loop: %d"), CurrentLoop);

	if (bTeleportPlayer)
	{
		TeleportPlayerToExit();
	}
	SetAnomalyDetected(false);
	GenerateAnomalyForNextLoop();
	ClearAllAIChats();
}

void ULoopManagerSubsystem::RegisterAIInteraction(int32 KindnessDelta, int32 SuspicionDelta, int32 DependencyDelta)
{
	if (ULoop9ObservationJournalSubsystem* Journal =
		GetGameInstance()->GetSubsystem<ULoop9ObservationJournalSubsystem>())
	{
		Journal->RecordAIInteraction();
	}

	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->RegisterAIInteraction();
		Relationship->RecordCall(CurrentLoop, KindnessDelta, SuspicionDelta, DependencyDelta);

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

void ULoopManagerSubsystem::ApplyAIDiagnosedDependencyDelta(int32 Delta)
{
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->ApplyAIDiagnosedDependencyDelta(Delta);
	}
}

void ULoopManagerSubsystem::RecordDragojloAdvice(
	EDragojloAdviceMode Mode,
	EDragojloLiftAdvice LiftAdvice,
	const FString& SuggestedZone,
	const FString& CommitmentId,
	int32 SuspicionDelta,
	int32 DependencyDelta)
{
	DragojloCommitmentTracker.ApplyAdvice(
		Mode,
		LiftAdvice,
		SuggestedZone,
		CommitmentId,
		SuspicionDelta,
		DependencyDelta,
		FPlatformTime::Seconds());
}

void ULoopManagerSubsystem::HandleObservationZoneEntered(FName ZoneId)
{
	if (!DragojloCommitmentTracker.OnZoneEntered(ZoneId, FPlatformTime::Seconds()))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Dragojlo decoy zone visited after %.2fs."),
		DragojloCommitmentTracker.GetState().DecoyVisitSeconds);
}

void ULoopManagerSubsystem::ResetRunState()
{
	CurrentLoop = 1;
	NotifyAIFriendsLoopChanged();
	bAnomalyDetected = false;
	bGameFinished = false;
	PendingElevatorDecisionId = 0;
	ActiveElevatorDecisionId = 0;
	bElevatorTransitionActive = false;
	bDeferredEndingPresentation = false;
	bDebugSecretExitIgnoresWall = false;
	DragojloCommitmentTracker.Reset();
	if (ULoop9ObservationJournalSubsystem* Journal =
		GetGameInstance()->GetSubsystem<ULoop9ObservationJournalSubsystem>())
	{
		Journal->ResetRun();
	}

	if (ULoop9AchievementsSubsystem* Achievements = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
	{
		Achievements->NotifyRunRestarted();
	}

	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->ResetRelationshipState();
	}

	if (UAnomalyManager* AnomalyManager = GetAnomalyManager(GetGameInstance()))
	{
		AnomalyManager->ResetAllAnomalies();
		AnomalyManager->ResetRunTracking();
	}

	ClearAllAIChats();
	UE_LOG(LogTemp, Log, TEXT("Loop: %d"), CurrentLoop);
}

void ULoopManagerSubsystem::RegisterLoopDecision(bool bWasCorrect, bool bAnomalyExisted, EButtonType ButtonType)
{
	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		Relationship->RegisterLoopDecision(bWasCorrect, bAnomalyExisted, ButtonType);
		Relationship->RecordLift(CurrentLoop, bWasCorrect, bAnomalyExisted);
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

bool ULoopManagerSubsystem::ApplyEndingTestSetup(ELoopEndingType EndingType)
{
#if UE_BUILD_SHIPPING
	(void)EndingType;
	return false;
#else
	URelationshipSubsystem* Relationship = GetRelationship();
	if (!Relationship)
	{
		UE_LOG(LogTemp, Warning, TEXT("EndingSetup: RelationshipSubsystem missing"));
		return false;
	}

	bGameFinished = false;
	bDeferredEndingPresentation = false;
	bElevatorTransitionActive = false;
	PendingElevatorDecisionId = 0;
	ActiveElevatorDecisionId = 0;
	CurrentLoop = 9;
	SetAnomalyDetected(false);

	Relationship->TotalResets = 0;
	// Already at the end-run threshold; next advance still bumps CurrentLoop 9 -> 10.
	Relationship->TotalAdvances = 9;

	switch (EndingType)
	{
	case ELoopEndingType::EscapeTogether:
		// Trust/kind/coop high, dependency low, stability high (avoids MergedMemory).
		Relationship->Trust = 0.75f;
		Relationship->Kindness = 0.65f;
		Relationship->Cooperation = 0.70f;
		Relationship->Suspicion = 0.25f;
		Relationship->Dependency = 0.40f;
		Relationship->AI_Stability = 0.85f;
		Relationship->TotalAIInteractions = 5;
		break;

	case ELoopEndingType::ObedientFool:
		Relationship->Trust = 0.65f;
		Relationship->Kindness = 0.50f;
		Relationship->Cooperation = 0.55f;
		Relationship->Suspicion = 0.25f;
		Relationship->Dependency = 0.60f;
		Relationship->AI_Stability = 0.75f;
		Relationship->TotalAIInteractions = 9;
		break;

	case ELoopEndingType::ColdBetrayal:
		Relationship->Trust = 0.70f;
		Relationship->Kindness = 0.30f;
		Relationship->Cooperation = 0.65f;
		Relationship->Suspicion = 0.35f;
		Relationship->Dependency = 0.45f;
		Relationship->AI_Stability = 0.75f;
		Relationship->TotalAIInteractions = 6;
		break;

	case ELoopEndingType::ParanoidSurvivor:
		Relationship->Trust = 0.30f;
		Relationship->Kindness = 0.40f;
		Relationship->Cooperation = 0.40f;
		Relationship->Suspicion = 0.70f;
		Relationship->Dependency = 0.20f;
		Relationship->AI_Stability = 0.60f;
		Relationship->TotalAIInteractions = 4;
		break;

	case ELoopEndingType::MergedMemory:
		Relationship->Trust = 0.55f;
		Relationship->Kindness = 0.65f;
		Relationship->Cooperation = 0.70f;
		Relationship->Suspicion = 0.30f;
		Relationship->Dependency = 0.45f;
		Relationship->AI_Stability = 0.65f;
		Relationship->TotalAIInteractions = 7;
		break;

	case ELoopEndingType::TheReplacement:
		Relationship->Trust = 0.70f;
		Relationship->Kindness = 0.70f;
		Relationship->Cooperation = 0.75f;
		Relationship->Suspicion = 0.25f;
		Relationship->Dependency = 0.70f;
		Relationship->AI_Stability = 0.60f;
		Relationship->TotalAIInteractions = 12;
		break;

	default:
		UE_LOG(LogTemp, Warning, TEXT("EndingSetup: unsupported ending type"));
		return false;
	}

	if (UAnomalyManager* AnomalyManager = GetGameInstance()->GetSubsystem<UAnomalyManager>())
	{
		AnomalyManager->ResetAllAnomalies();
	}

	NotifyAIFriendsLoopChanged();

	const ELoopEndingType Predicted = DetermineEndingType();
	UE_LOG(LogTemp, Log,
		TEXT("EndingSetup: CurrentLoop=%d | Trust=%.2f Kind=%.2f Coop=%.2f Susp=%.2f Dep=%.2f Stab=%.2f | AI=%d Adv=%d | predicted=%s (want %s). Press ADVANCE on elevator."),
		CurrentLoop,
		Relationship->Trust,
		Relationship->Kindness,
		Relationship->Cooperation,
		Relationship->Suspicion,
		Relationship->Dependency,
		Relationship->AI_Stability,
		Relationship->TotalAIInteractions,
		Relationship->TotalAdvances,
		*UEnum::GetValueAsString(Predicted),
		*UEnum::GetValueAsString(EndingType));

	return Predicted == EndingType;
#endif
}

void ULoopManagerSubsystem::TriggerEndingSequence()
{
	if (bGameFinished)
	{
		return;
	}

	if (ULoopEndingPresenterSubsystem* Presenter = GetGameInstance()->GetSubsystem<ULoopEndingPresenterSubsystem>())
	{
		bGameFinished = Presenter->TriggerEndingSequence(GetRelationship());
		if (!bGameFinished)
		{
			UE_LOG(LogTemp, Error, TEXT("Loop: ending presentation could not start; gameplay remains active."));
		}
	}
}

bool ULoopManagerSubsystem::IsSecretExitOpen() const
{
	if (bGameFinished || bElevatorTransitionActive || bDeferredEndingPresentation)
	{
		return false;
	}

	if (CurrentLoop < SecretExitMinLoop)
	{
		return false;
	}

#if !UE_BUILD_SHIPPING
	if (bDebugSecretExitIgnoresWall)
	{
		return true;
	}
#endif

	// The only honest way down is through the wall the Hide anomaly removed.
	const UAnomalyManager* AnomalyManager = GetAnomalyManager(GetGameInstance());
	return AnomalyManager && AnomalyManager->IsAnomalyTypeActive(ELoopAnomalyType::Hide);
}

bool ULoopManagerSubsystem::TryTriggerSecretExitEnding()
{
	if (!IsSecretExitOpen())
	{
		UE_LOG(LogTemp, Log, TEXT("Secret exit refused: loop=%d finished=%d"), CurrentLoop, bGameFinished ? 1 : 0);
		return false;
	}

	ULoopEndingPresenterSubsystem* Presenter = GetGameInstance()->GetSubsystem<ULoopEndingPresenterSubsystem>();
	if (!Presenter)
	{
		return false;
	}

	if (URelationshipSubsystem* Relationship = GetRelationship())
	{
		// He is not part of this ending; the archive still counts the floors walked.
		Relationship->TotalAdvances = FMath::Max(Relationship->TotalAdvances, CurrentLoop - 1);
	}

	bGameFinished = Presenter->TriggerForcedEnding(ELoopEndingType::TheExit, GetRelationship());
	if (!bGameFinished)
	{
		UE_LOG(LogTemp, Error, TEXT("Secret exit: ending presentation could not start; gameplay remains active."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Secret exit taken on loop %d -> The Exit"), CurrentLoop);
	}
	return bGameFinished;
}

void ULoopManagerSubsystem::RegisterAIFriend(AAI_Friend* AIFriend)
{
	if (!AIFriend)
	{
		return;
	}

	RegisteredAIFriends.RemoveAll([](const TWeakObjectPtr<AAI_Friend>& Ptr) { return !Ptr.IsValid(); });
	for (const TWeakObjectPtr<AAI_Friend>& Existing : RegisteredAIFriends)
	{
		if (Existing.Get() == AIFriend)
		{
			return;
		}
	}
	RegisteredAIFriends.Add(AIFriend);
}

void ULoopManagerSubsystem::UnregisterAIFriend(AAI_Friend* AIFriend)
{
	RegisteredAIFriends.RemoveAll([AIFriend](const TWeakObjectPtr<AAI_Friend>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == AIFriend;
	});
}

void ULoopManagerSubsystem::NotifyAIFriendsLoopChanged()
{
	RegisteredAIFriends.RemoveAll([](const TWeakObjectPtr<AAI_Friend>& Ptr) { return !Ptr.IsValid(); });
	for (const TWeakObjectPtr<AAI_Friend>& WeakFriend : RegisteredAIFriends)
	{
		if (AAI_Friend* AIFriend = WeakFriend.Get())
		{
			AIFriend->HandleLoopChanged();
		}
	}
}

void ULoopManagerSubsystem::RegisterTeleportPoint(ATeleportPoint* Point)
{
	if (!Point)
	{
		return;
	}

	PruneStaleTeleportCaches();

	TArray<TWeakObjectPtr<ATeleportPoint>>& Target =
		(Point->TeleportType == ECustomTeleportType::Entry) ? EntryPoints : ExitPoints;

	for (const TWeakObjectPtr<ATeleportPoint>& Existing : Target)
	{
		if (Existing.Get() == Point)
		{
			return;
		}
	}
	Target.Add(Point);
}

void ULoopManagerSubsystem::UnregisterTeleportPoint(ATeleportPoint* Point)
{
	auto RemovePoint = [Point](TArray<TWeakObjectPtr<ATeleportPoint>>& Points)
	{
		Points.RemoveAll([Point](const TWeakObjectPtr<ATeleportPoint>& Ptr)
		{
			return !Ptr.IsValid() || Ptr.Get() == Point;
		});
	};

	RemovePoint(EntryPoints);
	RemovePoint(ExitPoints);
}

void ULoopManagerSubsystem::PruneStaleTeleportCaches()
{
	EntryPoints.RemoveAll([](const TWeakObjectPtr<ATeleportPoint>& Ptr) { return !Ptr.IsValid(); });
	ExitPoints.RemoveAll([](const TWeakObjectPtr<ATeleportPoint>& Ptr) { return !Ptr.IsValid(); });
}

void ULoopManagerSubsystem::FindTeleportPoints()
{
	PruneStaleTeleportCaches();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (FallbackTeleportScanWorld.Get() == World)
	{
		return;
	}

	// One-time fallback for points that somehow skipped registration.
	FallbackTeleportScanWorld = World;
	TArray<AActor*> FoundPoints;
	UGameplayStatics::GetAllActorsOfClass(World, ATeleportPoint::StaticClass(), FoundPoints);

	for (AActor* Actor : FoundPoints)
	{
		if (ATeleportPoint* TeleportPoint = Cast<ATeleportPoint>(Actor))
		{
			RegisterTeleportPoint(TeleportPoint);
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
	if (ULoop9ObservationJournalSubsystem* Journal =
		GetGameInstance()->GetSubsystem<ULoop9ObservationJournalSubsystem>())
	{
		Journal->BeginFloor(CurrentLoop);
	}

	UAnomalyManager* AnomalyManager = GetAnomalyManager(GetGameInstance());
	if (!AnomalyManager)
	{
		return;
	}

	AnomalyManager->ResetAllAnomalies();
	AnomalyManager->BeginLoopVisit();

	// Floor 1 is always the clean baseline, including after an incorrect
	// decision sends the player back to it. Debug forcing remains available.
	if (CurrentLoop <= 1 && !bDebugAlwaysSpawnMoveAnomaly)
	{
		UE_LOG(LogTemp, Log, TEXT("Anomaly: NO (clean baseline)"));
		return;
	}

	const bool bShouldHaveAnomaly = bDebugAlwaysSpawnMoveAnomaly ? true : (FMath::FRand() < AnomalyChancePerLoop);
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
	RegisteredAIFriends.RemoveAll([](const TWeakObjectPtr<AAI_Friend>& Ptr) { return !Ptr.IsValid(); });

	for (const TWeakObjectPtr<AAI_Friend>& WeakFriend : RegisteredAIFriends)
	{
		if (const AAI_Friend* AIFriend = WeakFriend.Get())
		{
			if (UAI_ChatWidget* AIChatWidget = Cast<UAI_ChatWidget>(AIFriend->GetChatWidgetInstance()))
			{
				AIChatWidget->ClearChat();
			}
		}
	}
}
