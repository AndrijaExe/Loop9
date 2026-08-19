#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop/LoopTypes.h"
#include "Loop/LoopEndingEvaluator.h"
#include "RelationshipSubsystem.generated.h"

UCLASS()
class LOOP9_API URelationshipSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalResets = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalAdvances = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalAIInteractions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Trust = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Kindness = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Cooperation = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Suspicion = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float Dependency = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float AI_Stability = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "State")
	void RegisterAIInteraction();

	UFUNCTION(BlueprintCallable, Category = "State")
	void ApplyAIDiagnosedKindnessDelta(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "State")
	void ApplyAIDiagnosedSuspicionDelta(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "State")
	void ApplyAIDiagnosedDependencyDelta(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "State")
	void RegisterLoopDecision(bool bWasCorrect, bool bAnomalyExisted, EButtonType ButtonType);

	UFUNCTION(BlueprintCallable, Category = "State")
	void TickAIStabilityDecay();

	UFUNCTION(BlueprintCallable, Category = "State")
	void ResetRelationshipState();

	FEndingEvaluationContext BuildEndingContext() const;

	UPROPERTY(BlueprintReadOnly, Category = "Run Event")
	TArray<FRunEvent> RunEvents;

	const TArray<FRunEvent>& GetRunEvents() const { return RunEvents; }

	void RecordCall(int32 LoopIndex, int32 KindnessDelta = 0, int32 SuspicionDelta = 0, int32 DependencyDelta = 0);
	void RecordLift(int32 LoopIndex, bool bWasCorrect, bool bAnomalyExisted);
	void RecordEnding(int32 LoopIndex, ELoopEndingType EndingType);

	/** Leaving a floor: if the player never called, dependency drops. */
	void NotifyLoopLeft();

private:
	void ClampStateValues();
	void AppendEvent(FRunEvent Event);

	int32 AIInteractionsThisLoop = 0;
};
