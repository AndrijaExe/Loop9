#include "Anomaly/WatcherAnchor.h"

#include "Anomaly/WatcherAnomalyComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

AWatcherAnchor::AWatcherAnchor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrow->SetupAttachment(Root);
	FacingArrow->ArrowSize = 1.5f;
	FacingArrow->ArrowLength = 120.0f;
	FacingArrow->SetArrowColor(FLinearColor(0.85f, 0.15f, 0.15f));
	FacingArrow->bIsEditorOnly = true;

	Watcher = CreateDefaultSubobject<UWatcherAnomalyComponent>(TEXT("Watcher"));

	Tags.AddUnique(FName(TEXT("WatcherAnchor")));
}
