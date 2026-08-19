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
	TheReplacement UMETA(DisplayName = "The Replacement")
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
