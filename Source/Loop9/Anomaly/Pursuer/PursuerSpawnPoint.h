#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PursuerSpawnPoint.generated.h"

UCLASS()
class LOOP9_API APursuerSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APursuerSpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer Spawn")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pursuer Spawn", meta = (ClampMin = "0.01"))
	float Weight = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pursuer Spawn")
	TObjectPtr<class USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pursuer Spawn")
	TObjectPtr<class UArrowComponent> DirectionArrow;
};
