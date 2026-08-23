#include "Anomaly/AnomalyMovePoint.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

AAnomalyMovePoint::AAnomalyMovePoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	DirectionArrow->SetupAttachment(Root);
	DirectionArrow->ArrowSize = 1.0f;
}
