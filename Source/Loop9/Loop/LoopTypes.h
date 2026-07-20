#pragma once

#include "CoreMinimal.h"
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
