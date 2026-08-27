#include "Anomaly/LoopNumberAnomalyComponent.h"

#include "EngineUtils.h"
#include "Loop9.h"
#include "LoopNumberSign.h"

ULoopNumberAnomalyComponent::ULoopNumberAnomalyComponent()
{
	AnomalyProbability = 0.85f;
	AnomalyObjectKind = TEXT("the loop counter");
}

ALoopNumberSign* ULoopNumberAnomalyComponent::FindLoopNumberSign() const
{
	if (ALoopNumberSign* OwnerSign = Cast<ALoopNumberSign>(GetOwner()))
	{
		return OwnerSign;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ALoopNumberSign> It(World); It; ++It)
		{
			return *It;
		}
	}

	return nullptr;
}

bool ULoopNumberAnomalyComponent::ApplyAnomalyState()
{
	ALoopNumberSign* Sign = FindLoopNumberSign();
	if (!Sign)
	{
		UE_LOG(LogLoop9, Warning, TEXT("LoopNumber: skipped '%s' (no loop sign in the world)"),
			*GetNameSafe(GetOwner()));
		return false;
	}

	Sign->SetAnomalyGlitchActive(true);
	return true;
}

void ULoopNumberAnomalyComponent::RestoreNormalState()
{
	if (ALoopNumberSign* Sign = FindLoopNumberSign())
	{
		Sign->SetAnomalyGlitchActive(false);
	}
}
