#include "Subsystems/AnomalyManager.h"

#include "Anomaly/AnomalyTypes.h"
#include "Anomaly/MaterialSwapAnomalyComponent.h"
#include "Algo/Sort.h"
#include "Containers/Set.h"
#include "GameFramework/Actor.h"

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
			ForceActivateComponent(Component, INDEX_NONE);
			return Component->bIsAnomalyActive;
		}
	}

	return false;
}

bool UAnomalyManager::ForceActivateByType(ELoopAnomalyType Type)
{
	CleanupInvalidComponents();

	TArray<UAnomalyComponentBase*> Matches;
	for (TWeakObjectPtr<UAnomalyComponentBase> ComponentPtr : RegisteredComponents)
	{
		UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (Component && Component->GetAnomalyType() == Type)
		{
			Matches.Add(Component);
		}
	}

	if (Matches.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AnomalyManager: no registered anomalies of type %d"), static_cast<int32>(Type));
		return false;
	}

	UAnomalyComponentBase* Chosen = Matches[FMath::RandRange(0, Matches.Num() - 1)];
	ForceActivateComponent(Chosen, INDEX_NONE);
	return Chosen && Chosen->bIsAnomalyActive;
}

bool UAnomalyManager::ForceActivateByFilter(const FString& Filter, int32 MaterialIndex)
{
	CleanupInvalidComponents();

	if (Filter.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AnomalyManager: empty filter"));
		return false;
	}

	TArray<UAnomalyComponentBase*> Matches;
	for (TWeakObjectPtr<UAnomalyComponentBase> ComponentPtr : RegisteredComponents)
	{
		UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (Component && DoesComponentMatchFilter(Component, Filter))
		{
			Matches.Add(Component);
		}
	}

	if (Matches.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AnomalyManager: no anomalies matching filter '%s'"), *Filter);
		return false;
	}

	int32 Activated = 0;
	for (UAnomalyComponentBase* Chosen : Matches)
	{
		ForceActivateComponent(Chosen, MaterialIndex);
		if (Chosen && Chosen->bIsAnomalyActive)
		{
			++Activated;
			const AActor* Owner = Chosen->GetOwner();
			FString MatInfo;
			if (const UMaterialSwapAnomalyComponent* MaterialSwap = Cast<UMaterialSwapAnomalyComponent>(Chosen))
			{
				if (UMeshComponent* Mesh = Chosen->GetOwner()
					? Chosen->GetOwner()->FindComponentByClass<UMeshComponent>()
					: nullptr)
				{
					if (UMaterialInterface* Applied = Mesh->GetMaterial(MaterialSwap->MaterialSlot))
					{
						MatInfo = FString::Printf(TEXT(" applied=%s"), *Applied->GetName());
					}
				}
			}

			UE_LOG(LogTemp, Log, TEXT("AnomalyManager: forced '%s' on '%s'%s"),
				*Chosen->GetClass()->GetName(),
				Owner ? *Owner->GetActorNameOrLabel() : TEXT("None"),
				*MatInfo);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AnomalyManager: filter '%s' activated %d / %d"), *Filter, Activated, Matches.Num());
	return Activated > 0;
}

bool UAnomalyManager::DoesComponentMatchFilter(const UAnomalyComponentBase* Component, const FString& Filter)
{
	if (!Component)
	{
		return false;
	}

	const FString TypeLabel = Component->GetAnomalyTypeLabel().ToString();
	if (TypeLabel.Contains(Filter, ESearchCase::IgnoreCase))
	{
		return true;
	}

	// Allow short type names: Text, Move, Hide, ...
	const FString TypeName = UEnum::GetDisplayValueAsText(Component->GetAnomalyType()).ToString();
	if (TypeName.Contains(Filter, ESearchCase::IgnoreCase))
	{
		return true;
	}

	const FString ClassName = Component->GetClass()->GetName();
	if (ClassName.Contains(Filter, ESearchCase::IgnoreCase))
	{
		return true;
	}

	if (const AActor* Owner = Component->GetOwner())
	{
		if (Owner->GetName().Contains(Filter, ESearchCase::IgnoreCase)
			|| Owner->GetActorNameOrLabel().Contains(Filter, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

void UAnomalyManager::ForceActivateComponent(UAnomalyComponentBase* Component, int32 MaterialIndex)
{
	if (!Component)
	{
		return;
	}

	if (UMaterialSwapAnomalyComponent* MaterialSwap = Cast<UMaterialSwapAnomalyComponent>(Component))
	{
		if (MaterialIndex >= 0)
		{
			MaterialSwap->SetForcedMaterialIndex(MaterialIndex);
		}
	}

	if (Component->bIsAnomalyActive)
	{
		Component->DeactivateAnomaly();
	}

	Component->ActivateAnomaly();
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
	CleanupInvalidComponents();

	UE_LOG(LogTemp, Log, TEXT("===== Anomaly Stats (%d registered, %d active) ====="),
		RegisteredComponents.Num(), GetActiveAnomalyCount());

	int32 Index = 0;
	for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
	{
		UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (!Component)
		{
			continue;
		}

		const AActor* Owner = Component->GetOwner();
		FString Extra;
		if (const UMaterialSwapAnomalyComponent* MaterialSwap = Cast<UMaterialSwapAnomalyComponent>(Component))
		{
			Extra = FString::Printf(TEXT(" mats=%d"), MaterialSwap->AnomalyMaterials.Num());
		}

		UE_LOG(LogTemp, Log, TEXT("[%d] %s | %s | %s | active=%d | p=%.2f%s"),
			Index++,
			Owner ? *Owner->GetActorNameOrLabel() : TEXT("None"),
			*Component->GetAnomalyTypeLabel().ToString(),
			*Component->GetClass()->GetName(),
			Component->bIsAnomalyActive ? 1 : 0,
			Component->AnomalyProbability,
			*Extra);
	}

	UE_LOG(LogTemp, Log, TEXT("Commands: AnomalyList | AnomalyReset | AnomalyForce <filter> [matIndex] | AnomalyForceAny"));
}
