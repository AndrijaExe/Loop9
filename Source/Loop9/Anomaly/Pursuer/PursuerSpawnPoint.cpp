#include "Anomaly/Pursuer/PursuerSpawnPoint.h"
#include "Components/SceneComponent.h"
#include "Components/ArrowComponent.h"

APursuerSpawnPoint::APursuerSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	DirectionArrow->SetupAttachment(Root);
	DirectionArrow->ArrowSize = 1.0f;
}
