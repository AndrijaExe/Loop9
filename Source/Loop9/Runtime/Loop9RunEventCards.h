#pragma once

#include "CoreMinimal.h"
#include "Loop/LoopTypes.h"

namespace Loop9RunEventCards
{
	FRunEventCard Build(const FRunEvent& Event);
	TArray<FRunEventCard> BuildAll(const TArray<FRunEvent>& Events);
	FLinearColor RingColor(const FRunEvent& Event);
}
