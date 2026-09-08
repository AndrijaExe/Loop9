#pragma once

#include "CoreMinimal.h"
#include "Math/Color.h"
#include "LoopTypes.generated.h"

UENUM(BlueprintType)
enum class ELoopEndingType : uint8
{
	EscapeTogether UMETA(DisplayName = "Escape Together"),
	ObedientFool UMETA(DisplayName = "Obedient Fool"),
	ColdBetrayal UMETA(DisplayName = "Cold Betrayal"),
	ParanoidSurvivor UMETA(DisplayName = "Paranoid Survivor"),
	MergedMemory UMETA(DisplayName = "Merged Memory"),
	TheReplacement UMETA(DisplayName = "The Replacement"),
	/**
	 * 1.1 secret ending: the player walks out through the ground-floor door
	 * behind a wall the Hide anomaly removed. Triggered, never scored.
	 */
	TheExit UMETA(DisplayName = "The Exit")
};

UENUM(BlueprintType)
enum class EButtonType : uint8
{
	Increment UMETA(DisplayName = "Advance Loop (Next Floor)"),
	Reset UMETA(DisplayName = "Reset Loop (Previous Floor)")
};

UENUM()
enum class ELoopAction : uint8
{
	Advance,
	Reset
};

UENUM(BlueprintType)
enum class ERunEventType : uint8
{
	Call UMETA(DisplayName = "Call"),
	CorrectLift UMETA(DisplayName = "Correct Lift"),
	WrongLift UMETA(DisplayName = "Wrong Lift"),
	Ending UMETA(DisplayName = "Ending")
};

UENUM(BlueprintType)
enum class ERunEventTone : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Friendly UMETA(DisplayName = "Friendly"),
	Hostile UMETA(DisplayName = "Hostile"),
	Suspicious UMETA(DisplayName = "Suspicious")
};

/**
 * Server-authored advice modes for Dragojlo's per-run commitment system.
 * Wire names match backend AdviceDirective::mode().
 */
UENUM(BlueprintType)
enum class EDragojloAdviceMode : uint8
{
	None UMETA(DisplayName = "None"),
	Withhold UMETA(DisplayName = "Withhold"),
	AccurateHint UMETA(DisplayName = "Accurate Hint"),
	MisdirectLocation UMETA(DisplayName = "Misdirect Location"),
	Confrontation UMETA(DisplayName = "Confrontation"),
	WrongLift UMETA(DisplayName = "Wrong Lift"),
	AccurateLift UMETA(DisplayName = "Accurate Lift"),
	/** 1.1: he describes the previous floor's anomaly place as if it were this one. */
	StaleFloor UMETA(DisplayName = "Stale Floor")
};

/** Structured lift recommendation, never parsed from natural-language reply text. */
UENUM(BlueprintType)
enum class EDragojloLiftAdvice : uint8
{
	None UMETA(DisplayName = "None"),
	Lit UMETA(DisplayName = "Lit"),
	Dark UMETA(DisplayName = "Dark")
};

/**
 * Per-run memory of what Dragojlo committed to. Cleared on ResetRunState only —
 * not saved, not Clouded, and never stores raw chat text.
 */
USTRUCT(BlueprintType)
struct FDragojloCommitmentState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	EDragojloAdviceMode LastAdviceMode = EDragojloAdviceMode::None;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	EDragojloLiftAdvice LastLiftAdvice = EDragojloLiftAdvice::None;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	FString LastSuggestedZone;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	FString LastCommitmentId;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	bool bLocationMisdirectionUsed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	bool bContradictionExposed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	bool bPendingDecisionSurrender = false;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	bool bWrongLiftUsed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	bool bFollowedLastLiftAdvice = false;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	bool bVisitedSuggestedDecoy = false;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	bool bConfrontationResponseUsed = false;

	/** 1.1: the one-shot "previous floor's place" slip has been spent this run. */
	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	bool bStaleFloorUsed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	int32 LiftAdviceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	int32 FollowedLiftAdviceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	int32 WrongLiftAdviceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	int32 FollowedWrongLiftAdviceCount = 0;

	/** Seconds from the planted location hint to entering its authored zone; -1 when unvisited. */
	UPROPERTY(BlueprintReadOnly, Category = "Commitment")
	float DecoyVisitSeconds = -1.0f;
};

/** In-memory beat for the post-run timeline. Cleared on each new shift. */
USTRUCT(BlueprintType)
struct FRunEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	ERunEventType Type = ERunEventType::Call;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	int32 LoopIndex = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	int32 Count = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	int32 KindnessDelta = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	int32 SuspicionDelta = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	int32 DependencyDelta = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	ERunEventTone Tone = ERunEventTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	bool bAnomalyExisted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	ELoopEndingType EndingType = ELoopEndingType::ParanoidSurvivor;
};

/** Localized timeline row. Ending WBP should spawn from these, not write copy. */
USTRUCT(BlueprintType)
struct FRunEventCard
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	ERunEventType Type = ERunEventType::Call;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	ERunEventTone Tone = ERunEventTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	int32 LoopIndex = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	int32 Count = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	FText Body;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	FLinearColor RingColor = FLinearColor(0.50f, 0.80f, 1.00f);
};

USTRUCT()
struct FLoopElevatorDecision
{
	GENERATED_BODY()

	UPROPERTY()
	int32 DecisionId = 0;

	UPROPERTY()
	EButtonType ButtonType = EButtonType::Increment;

	UPROPERTY()
	ELoopAction Action = ELoopAction::Advance;

	UPROPERTY()
	bool bAnomaliesExisted = false;

	UPROPERTY()
	bool bWasCorrect = false;

	bool IsValid() const { return DecisionId > 0; }
};
