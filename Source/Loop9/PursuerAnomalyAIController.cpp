#include "PursuerAnomalyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

APursuerAnomalyAIController::APursuerAnomalyAIController()
{
}

void APursuerAnomalyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
      // In this project setup, run the Behavior Tree from BP controller graph
		// (Run Behavior Tree node) after possess/begin play.
		bBehaviorTreeRunning = false;
	}
	else
	{
		bBehaviorTreeRunning = false;
	}
}

void APursuerAnomalyAIController::SetTargetActor(AActor* TargetActor)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(TargetActorKeyName, TargetActor);
	}
}

void APursuerAnomalyAIController::SetObservationState(bool bPlayerLookingAtPursuer)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(PlayerLookingAtPursuerKeyName, bPlayerLookingAtPursuer);
		BB->SetValueAsBool(ShouldChaseKeyName, !bPlayerLookingAtPursuer);
	}
}

void APursuerAnomalyAIController::StopChaseMovement()
{
	StopMovement();
}
