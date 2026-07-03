#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PursuerAnomalyAIController.generated.h"

UCLASS()
class LOOP9_API APursuerAnomalyAIController : public AAIController
{
	GENERATED_BODY()

public:
	APursuerAnomalyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	TObjectPtr<class UBlackboardData> BlackboardAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	TObjectPtr<class UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI|Blackboard")
	FName TargetActorKeyName = TEXT("TargetActor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI|Blackboard")
	FName ShouldChaseKeyName = TEXT("bShouldChase");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI|Blackboard")
	FName PlayerLookingAtPursuerKeyName = TEXT("bPlayerLookingAtPursuer");

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetTargetActor(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetObservationState(bool bPlayerLookingAtPursuer);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StopChaseMovement();

	UFUNCTION(BlueprintPure, Category = "AI")
	bool IsUsingBehaviorTree() const { return bBehaviorTreeRunning; }

private:
	bool bBehaviorTreeRunning = false;
};
