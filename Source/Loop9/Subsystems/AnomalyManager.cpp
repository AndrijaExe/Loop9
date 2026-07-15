#include "Subsystems/AnomalyManager.h"

#include "Anomaly/AnomalyTypes.h"
#include "Algo/Sort.h"
#include "Containers/Set.h"

namespace
{
	struct FCandidate
	{
		UAnomalyComponentBase* Component = nullptr;
		float Probability = 0.0f;
	};

	constexpr bool bDebugOnlyMoveAnomaly = false;

	constexpr ELoopAnomalyType AnomalyTypeOrder[] =
	{
		ELoopAnomalyType::Hide,
		ELoopAnomalyType::Move,
		ELoopAnomalyType::Light,
		ELoopAnomalyType::Audio,
		ELoopAnomalyType::Text,
		ELoopAnomalyType::DoorLock,
		ELoopAnomalyType::Pursuer,
		ELoopAnomalyType::Scale,
		ELoopAnomalyType::Clock,
		ELoopAnomalyType::PhantomMessage
	};

	int32 GetTypeIndex(ELoopAnomalyType Type)
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(AnomalyTypeOrder); ++Index)
		{
			if (AnomalyTypeOrder[Index] == Type)
			{
				return Index;
			}
		}

		return INDEX_NONE;
	}
}

void UAnomalyManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAnomalyManager::Deinitialize()
{
	Super::Deinitialize();
	RegisteredComponents.Empty();
	TrackedLoopIndex = INDEX_NONE;
	PreviousLoopAnomalyKey.Empty();
	CurrentLoopAnomalyKey.Empty();
	CurrentLoopAnomalyContext = TEXT("No active anomaly currently detected.");
	bCurrentLoopAnomalyRepeat = false;
}

void UAnomalyManager::CleanupInvalidComponents()
{
	RegisteredComponents.RemoveAll([](const TWeakObjectPtr<UAnomalyComponentBase>& Component)
	{
		return !Component.IsValid();
	});
}

void UAnomalyManager::RegisterAnomalyComponent(UAnomalyComponentBase* Component)
{
	if (!Component || RegisteredComponents.Contains(Component))
	{
		return;
	}

	RegisteredComponents.Add(Component);
}

void UAnomalyManager::UnregisterAnomalyComponent(UAnomalyComponentBase* Component)
{
	if (Component)
	{
		RegisteredComponents.Remove(Component);
	}
}

void UAnomalyManager::UpdateLoopAnomalyTracking(int32 LoopIndex)
{
	if (TrackedLoopIndex == LoopIndex)
	{
		return;
	}

	FString NewKey;
	FString NewContext;
	ComputeActiveAnomalySnapshot(NewKey, NewContext);

	bCurrentLoopAnomalyRepeat = !PreviousLoopAnomalyKey.IsEmpty() && PreviousLoopAnomalyKey == NewKey;
	CurrentLoopAnomalyKey = NewKey;
	CurrentLoopAnomalyContext = NewContext;
	PreviousLoopAnomalyKey = NewKey;
	TrackedLoopIndex = LoopIndex;
}

void UAnomalyManager::ComputeActiveAnomalySnapshot(FString& OutKey, FString& OutContext) const
{
	TArray<FString> Labels;

	for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
	{
		const UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (!Component || !Component->bIsAnomalyActive)
		{
			continue;
		}

		Labels.Add(Component->GetAnomalyTypeLabel().ToString());
	}

	if (Labels.Num() == 0)
	{
		OutKey = TEXT("none");
		OutContext = TEXT("No active anomaly currently detected.");
		return;
	}

	Labels.Sort();
	TSet<FString> Seen;
	TArray<FString> UniqueLabels;
	for (const FString& Label : Labels)
	{
		if (!Seen.Contains(Label))
		{
			Seen.Add(Label);
			UniqueLabels.Add(Label);
		}
	}

	OutKey = FString::Join(UniqueLabels, TEXT("|"));
	OutContext = FString::Printf(TEXT("Active anomaly types: %s"), *FString::Join(UniqueLabels, TEXT(", ")));
}

bool UAnomalyManager::ForceActivateAnyAnomaly()
{
	CleanupInvalidComponents();

	for (TWeakObjectPtr<UAnomalyComponentBase> ComponentPtr : RegisteredComponents)
	{
		UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (!Component || Component->bIsAnomalyActive)
		{
			continue;
		}

		const ELoopAnomalyType Type = Component->GetAnomalyType();
		if (bDebugOnlyMoveAnomaly && Type != ELoopAnomalyType::Move)
		{
			continue;
		}

		if (GetTypeIndex(Type) != INDEX_NONE)
		{
			Component->ActivateAnomaly();
			return true;
		}
	}

	return false;
}

void UAnomalyManager::TriggerRandomAnomalies(int32 Count, float MinProbability)
{
	CleanupInvalidComponents();

	if (RegisteredComponents.Num() == 0)
	{
		return;
	}

	TArray<FCandidate> CandidatePools[UE_ARRAY_COUNT(AnomalyTypeOrder)];
	for (TWeakObjectPtr<UAnomalyComponentBase> ComponentPtr : RegisteredComponents)
	{
		UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (!Component || Component->bIsAnomalyActive || Component->AnomalyProbability < MinProbability)
		{
			continue;
		}

		const int32 TypeIndex = GetTypeIndex(Component->GetAnomalyType());
		if (TypeIndex != INDEX_NONE)
		{
			CandidatePools[TypeIndex].Add({ Component, Component->AnomalyProbability });
		}
	}

	if (bDebugOnlyMoveAnomaly)
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(AnomalyTypeOrder); ++Index)
		{
			if (AnomalyTypeOrder[Index] != ELoopAnomalyType::Move)
			{
				CandidatePools[Index].Empty();
			}
		}
	}

	bool bHasAnyCandidate = false;
	for (const TArray<FCandidate>& Pool : CandidatePools)
	{
		if (Pool.Num() > 0)
		{
			bHasAnyCandidate = true;
			break;
		}
	}

	if (!bHasAnyCandidate)
	{
		return;
	}

	int32 AnomaliesTriggered = 0;
	const int32 MaxAttempts = FMath::Max(Count * 4, 8);
	int32 Attempts = 0;

	while (AnomaliesTriggered < Count && Attempts < MaxAttempts)
	{
		Attempts++;

		TArray<int32> AvailableTypeIndices;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(AnomalyTypeOrder); ++Index)
		{
			if (CandidatePools[Index].Num() > 0)
			{
				AvailableTypeIndices.Add(Index);
			}
		}

		if (AvailableTypeIndices.Num() == 0)
		{
			break;
		}

		const int32 ChosenTypeIndex = AvailableTypeIndices[FMath::RandRange(0, AvailableTypeIndices.Num() - 1)];
		TArray<FCandidate>& ChosenPool = CandidatePools[ChosenTypeIndex];

		const int32 RandomIndex = FMath::RandRange(0, ChosenPool.Num() - 1);
		const FCandidate Selected = ChosenPool[RandomIndex];
		ChosenPool.RemoveAt(RandomIndex);

		if (Selected.Component)
		{
			const float Roll = FMath::FRand();
			if (Roll <= Selected.Probability)
			{
				Selected.Component->ActivateAnomaly();
				if (Selected.Component->bIsAnomalyActive)
				{
					AnomaliesTriggered++;
				}
			}
		}
	}
}

void UAnomalyManager::ResetAllAnomalies()
{
	CleanupInvalidComponents();

	for (TWeakObjectPtr<UAnomalyComponentBase> ComponentPtr : RegisteredComponents)
	{
		UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (!Component)
		{
			continue;
		}

		if (Component->bIsAnomalyActive)
		{
			Component->DeactivateAnomaly();
		}
		else
		{
			Component->ResetToNormalState();
		}
	}
}

int32 UAnomalyManager::GetActiveAnomalyCount() const
{
	int32 ActiveCount = 0;
	for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
	{
		const UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (Component && Component->bIsAnomalyActive)
		{
			ActiveCount++;
		}
	}

	return ActiveCount;
}

void UAnomalyManager::PrintAnomalyStats()
{
}
