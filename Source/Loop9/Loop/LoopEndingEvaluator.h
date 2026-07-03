#pragma once

#include "CoreMinimal.h"
#include "Loop/LoopTypes.h"

struct FRelationshipState
{
	float Trust = 0.5f;
	float Kindness = 0.5f;
	float Cooperation = 0.5f;
	float Suspicion = 0.2f;
	float Dependency = 0.2f;
	float AIStability = 1.0f;
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

class LOOP9_API FLoopEndingEvaluator
{
public:
	static ELoopEndingType Evaluate(const FEndingEvaluationContext& Context);
};
