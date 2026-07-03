// Fill out your copyright notice in the Description page of Project Settings.

#include "AnomalyManager.h"
#include "AnomalyComponent.h"
#include "MoveAnomalyComponent.h"
#include "LightFlickerAnomalyComponent.h"
#include "AudioAnomalyComponent.h"
#include "TextSpawnAnomalyComponent.h"
#include "DoorLockStateAnomalyComponent.h"
#include "PursuerAnomalyComponent.h"
#include "Algo/Sort.h"
#include "Containers/Set.h"

namespace
{
	enum class EAnomalyType : uint8
	{
		Hide = 0,
		Move,
		Light,
		Audio,
		Text,
		DoorLock,
		Pursuer,
		Count
	};

	struct FCandidate
	{
		AActor* Actor = nullptr;
		float Probability = 0.0f;
	};

	constexpr bool bDebugOnlyMoveAnomaly = false;
	constexpr int32 AnomalyTypeCount = static_cast<int32>(EAnomalyType::Count);
	constexpr EAnomalyType AnomalyTypeOrder[] =
	{
		EAnomalyType::Hide,
		EAnomalyType::Move,
		EAnomalyType::Light,
		EAnomalyType::Audio,
		EAnomalyType::Text,
		EAnomalyType::DoorLock,
		EAnomalyType::Pursuer
	};

	template<typename TComponent>
	TComponent* FindAnomalyComponent(AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<TComponent>() : nullptr;
	}

	template<typename TComponent>
	const TComponent* FindAnomalyComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<TComponent>() : nullptr;
	}

	template<typename TComponent>
	bool IsAnomalyTypePresent(const AActor* Actor)
	{
		return FindAnomalyComponent<TComponent>(Actor) != nullptr;
	}

	template<typename TComponent>
	bool IsAnomalyTypeActive(const AActor* Actor)
	{
		if (const TComponent* Component = FindAnomalyComponent<TComponent>(Actor))
		{
			return Component->bIsAnomalyActive;
		}

		return false;
	}

	template<typename TComponent>
	bool TryActivateAnomalyType(AActor* Actor)
	{
		if (TComponent* Component = FindAnomalyComponent<TComponent>(Actor))
		{
			if (!Component->bIsAnomalyActive)
			{
				Component->ActivateAnomaly();
				return true;
			}
		}

		return false;
	}

	template<typename TComponent>
	void DeactivateAnomalyTypeIfActive(AActor* Actor)
	{
		if (TComponent* Component = FindAnomalyComponent<TComponent>(Actor))
		{
			if (Component->bIsAnomalyActive)
			{
				Component->DeactivateAnomaly();
			}
		}
	}

	template<typename TComponent>
	void AddCandidateForType(AActor* Actor, float MinProbability, TArray<FCandidate>& OutCandidates)
	{
		if (const TComponent* Component = FindAnomalyComponent<TComponent>(Actor))
		{
			if (!Component->bIsAnomalyActive && Component->AnomalyProbability >= MinProbability)
			{
				OutCandidates.Add({ Actor, Component->AnomalyProbability });
			}
		}
	}

	int32 GetTypeIndex(EAnomalyType Type)
	{
		return static_cast<int32>(Type);
	}

	bool HasAnomalyType(const AActor* Actor, EAnomalyType Type)
	{
		switch (Type)
		{
		case EAnomalyType::Hide: return IsAnomalyTypePresent<UAnomalyComponent>(Actor);
		case EAnomalyType::Move: return IsAnomalyTypePresent<UMoveAnomalyComponent>(Actor);
		case EAnomalyType::Light: return IsAnomalyTypePresent<ULightFlickerAnomalyComponent>(Actor);
		case EAnomalyType::Audio: return IsAnomalyTypePresent<UAudioAnomalyComponent>(Actor);
		case EAnomalyType::Text: return IsAnomalyTypePresent<UTextSpawnAnomalyComponent>(Actor);
		case EAnomalyType::DoorLock: return IsAnomalyTypePresent<UDoorLockStateAnomalyComponent>(Actor);
		case EAnomalyType::Pursuer: return IsAnomalyTypePresent<UPursuerAnomalyComponent>(Actor);
		default: return false;
		}
	}

	bool IsTypeActive(const AActor* Actor, EAnomalyType Type)
	{
		switch (Type)
		{
		case EAnomalyType::Hide: return IsAnomalyTypeActive<UAnomalyComponent>(Actor);
		case EAnomalyType::Move: return IsAnomalyTypeActive<UMoveAnomalyComponent>(Actor);
		case EAnomalyType::Light: return IsAnomalyTypeActive<ULightFlickerAnomalyComponent>(Actor);
		case EAnomalyType::Audio: return IsAnomalyTypeActive<UAudioAnomalyComponent>(Actor);
		case EAnomalyType::Text: return IsAnomalyTypeActive<UTextSpawnAnomalyComponent>(Actor);
		case EAnomalyType::DoorLock: return IsAnomalyTypeActive<UDoorLockStateAnomalyComponent>(Actor);
		case EAnomalyType::Pursuer: return IsAnomalyTypeActive<UPursuerAnomalyComponent>(Actor);
		default: return false;
		}
	}

	bool TryActivateType(AActor* Actor, EAnomalyType Type)
	{
		switch (Type)
		{
		case EAnomalyType::Hide: return TryActivateAnomalyType<UAnomalyComponent>(Actor);
		case EAnomalyType::Move: return TryActivateAnomalyType<UMoveAnomalyComponent>(Actor);
		case EAnomalyType::Light: return TryActivateAnomalyType<ULightFlickerAnomalyComponent>(Actor);
		case EAnomalyType::Audio: return TryActivateAnomalyType<UAudioAnomalyComponent>(Actor);
		case EAnomalyType::Text: return TryActivateAnomalyType<UTextSpawnAnomalyComponent>(Actor);
		case EAnomalyType::DoorLock: return TryActivateAnomalyType<UDoorLockStateAnomalyComponent>(Actor);
		case EAnomalyType::Pursuer: return TryActivateAnomalyType<UPursuerAnomalyComponent>(Actor);
		default: return false;
		}
	}

	void AddTypeCandidate(AActor* Actor, EAnomalyType Type, float MinProbability, TArray<FCandidate>& OutCandidates)
	{
		switch (Type)
		{
		case EAnomalyType::Hide: AddCandidateForType<UAnomalyComponent>(Actor, MinProbability, OutCandidates); break;
		case EAnomalyType::Move: AddCandidateForType<UMoveAnomalyComponent>(Actor, MinProbability, OutCandidates); break;
		case EAnomalyType::Light: AddCandidateForType<ULightFlickerAnomalyComponent>(Actor, MinProbability, OutCandidates); break;
		case EAnomalyType::Audio: AddCandidateForType<UAudioAnomalyComponent>(Actor, MinProbability, OutCandidates); break;
		case EAnomalyType::Text: AddCandidateForType<UTextSpawnAnomalyComponent>(Actor, MinProbability, OutCandidates); break;
		case EAnomalyType::DoorLock: AddCandidateForType<UDoorLockStateAnomalyComponent>(Actor, MinProbability, OutCandidates); break;
		case EAnomalyType::Pursuer: AddCandidateForType<UPursuerAnomalyComponent>(Actor, MinProbability, OutCandidates); break;
		default: break;
		}
	}

	void DeactivateTypeIfActive(AActor* Actor, EAnomalyType Type)
	{
		switch (Type)
		{
		case EAnomalyType::Hide: DeactivateAnomalyTypeIfActive<UAnomalyComponent>(Actor); break;
		case EAnomalyType::Move: DeactivateAnomalyTypeIfActive<UMoveAnomalyComponent>(Actor); break;
		case EAnomalyType::Light: DeactivateAnomalyTypeIfActive<ULightFlickerAnomalyComponent>(Actor); break;
		case EAnomalyType::Audio: DeactivateAnomalyTypeIfActive<UAudioAnomalyComponent>(Actor); break;
		case EAnomalyType::Text: DeactivateAnomalyTypeIfActive<UTextSpawnAnomalyComponent>(Actor); break;
		case EAnomalyType::DoorLock: DeactivateAnomalyTypeIfActive<UDoorLockStateAnomalyComponent>(Actor); break;
		case EAnomalyType::Pursuer: DeactivateAnomalyTypeIfActive<UPursuerAnomalyComponent>(Actor); break;
		default: break;
		}
	}

	FString GetTypeLabel(EAnomalyType Type)
	{
		switch (Type)
		{
		case EAnomalyType::Hide: return TEXT("HideAnomaly");
		case EAnomalyType::Move: return TEXT("MoveAnomaly");
		case EAnomalyType::Light: return TEXT("LightFlickerAnomaly");
		case EAnomalyType::Audio: return TEXT("AudioAnomaly");
		case EAnomalyType::Text: return TEXT("TextAnomaly");
		case EAnomalyType::DoorLock: return TEXT("DoorLockAnomaly");
		case EAnomalyType::Pursuer: return TEXT("PursuerAnomaly");
		default: return TEXT("UnknownAnomaly");
		}
	}

	void RestoreHiddenActorStateFromHideAnomaly(AActor* Actor)
	{
		if (!Actor)
		{
			return;
		}

		Actor->SetActorHiddenInGame(false);
		Actor->SetActorEnableCollision(true);
		Actor->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));

		TArray<UPrimitiveComponent*> PrimitiveComponents;
		Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			PrimitiveComponent->SetVisibility(true, true);
			PrimitiveComponent->SetHiddenInGame(false);
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}
}

void UAnomalyManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

bool UAnomalyManager::ForceActivateAnyAnomaly()
{
	RegisteredAnomalies.RemoveAll([](const TWeakObjectPtr<AActor>& Actor) { return !Actor.IsValid(); });

	const EAnomalyType FirstType = bDebugOnlyMoveAnomaly ? EAnomalyType::Move : EAnomalyType::Hide;
	const EAnomalyType LastTypeExclusive = bDebugOnlyMoveAnomaly ? EAnomalyType::Light : EAnomalyType::Count;

	for (TWeakObjectPtr<AActor> ActorPtr : RegisteredAnomalies)
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			for (int32 TypeIndex = GetTypeIndex(FirstType); TypeIndex < GetTypeIndex(LastTypeExclusive); ++TypeIndex)
			{
				if (TryActivateType(Actor, static_cast<EAnomalyType>(TypeIndex)))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void UAnomalyManager::Deinitialize()
{
	Super::Deinitialize();
	RegisteredAnomalies.Empty();
	TrackedLoopIndex = INDEX_NONE;
	PreviousLoopAnomalyKey.Empty();
	CurrentLoopAnomalyKey.Empty();
	CurrentLoopAnomalyContext = TEXT("No active anomaly currently detected.");
	bCurrentLoopAnomalyRepeat = false;
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

	for (const TWeakObjectPtr<AActor>& ActorPtr : RegisteredAnomalies)
	{
		AActor* Actor = ActorPtr.Get();
		if (!Actor)
		{
			continue;
		}

		for (EAnomalyType Type : AnomalyTypeOrder)
		{
			if (IsTypeActive(Actor, Type))
			{
				Labels.Add(GetTypeLabel(Type));
			}
		}
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

void UAnomalyManager::RegisterAnomaly(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	// Check if already registered
	if (RegisteredAnomalies.Contains(Actor))
	{
		return;
	}

	for (EAnomalyType Type : AnomalyTypeOrder)
	{
		if (HasAnomalyType(Actor, Type))
		{
			RegisteredAnomalies.Add(Actor);
			return;
		}
	}
}

void UAnomalyManager::UnregisterAnomaly(AActor* Actor)
{
	if (Actor)
	{
		RegisteredAnomalies.Remove(Actor);
	}
}

void UAnomalyManager::TriggerRandomAnomalies(int32 Count, float MinProbability)
{
	// Clean up invalid references
	RegisteredAnomalies.RemoveAll([](const TWeakObjectPtr<AActor>& Actor) { return !Actor.IsValid(); });

	if (RegisteredAnomalies.Num() == 0)
	{
		return;
	}

	TArray<FCandidate> CandidatePools[AnomalyTypeCount];
	for (TWeakObjectPtr<AActor> ActorPtr : RegisteredAnomalies)
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			for (EAnomalyType Type : AnomalyTypeOrder)
			{
				AddTypeCandidate(Actor, Type, MinProbability, CandidatePools[GetTypeIndex(Type)]);
			}
		}
	}

	if (bDebugOnlyMoveAnomaly)
	{
		for (EAnomalyType Type : AnomalyTypeOrder)
		{
			if (Type != EAnomalyType::Move)
			{
				CandidatePools[GetTypeIndex(Type)].Empty();
			}
		}
	}

	bool bHasAnyCandidate = false;
	for (EAnomalyType Type : AnomalyTypeOrder)
	{
		if (CandidatePools[GetTypeIndex(Type)].Num() > 0)
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

		TArray<EAnomalyType> AvailableTypes;
		for (EAnomalyType Type : AnomalyTypeOrder)
		{
			if (CandidatePools[GetTypeIndex(Type)].Num() > 0)
			{
				AvailableTypes.Add(Type);
			}
		}

		if (AvailableTypes.Num() == 0)
		{
			break;
		}

		const EAnomalyType ChosenType = AvailableTypes[FMath::RandRange(0, AvailableTypes.Num() - 1)];
		TArray<FCandidate>& ChosenPool = CandidatePools[GetTypeIndex(ChosenType)];

		if (ChosenPool.Num() == 0)
		{
			continue;
		}

		const int32 RandomIndex = FMath::RandRange(0, ChosenPool.Num() - 1);
		const FCandidate Selected = ChosenPool[RandomIndex];
		ChosenPool.RemoveAt(RandomIndex);

		if (Selected.Actor)
		{
			const float Roll = FMath::FRand();
			if (Roll <= Selected.Probability)
			{
				if (TryActivateType(Selected.Actor, ChosenType))
				{
					AnomaliesTriggered++;
				}
			}
		}
	}
}

void UAnomalyManager::ResetAllAnomalies()
{
	// Clean up invalid references
	RegisteredAnomalies.RemoveAll([](const TWeakObjectPtr<AActor>& Actor) { return !Actor.IsValid(); });

	for (TWeakObjectPtr<AActor> ActorPtr : RegisteredAnomalies)
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			if (UAnomalyComponent* HideAnomaly = FindAnomalyComponent<UAnomalyComponent>(Actor))
			{
				if (HideAnomaly->bIsAnomalyActive)
				{
					HideAnomaly->DeactivateAnomaly();
				}
				else
				{
					RestoreHiddenActorStateFromHideAnomaly(HideAnomaly->GetOwner());
				}
			}

			for (EAnomalyType Type : AnomalyTypeOrder)
			{
				if (Type != EAnomalyType::Hide)
				{
					DeactivateTypeIfActive(Actor, Type);
				}
			}
		}
	}
}

int32 UAnomalyManager::GetActiveAnomalyCount() const
{
	int32 ActiveCount = 0;
	for (const TWeakObjectPtr<AActor>& ActorPtr : RegisteredAnomalies)
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			for (EAnomalyType Type : AnomalyTypeOrder)
			{
				if (IsTypeActive(Actor, Type))
				{
					ActiveCount++;
					break;
				}
			}
		}
	}
	return ActiveCount;
}

void UAnomalyManager::PrintAnomalyStats()
{
  // intentionally minimal (log spam reduced)
}

