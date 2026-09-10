#include "Subsystems/AnomalyManager.h"

#include "Loop9.h"
#include "Anomaly/AnomalyTypes.h"
#include "Anomaly/MaterialSwapAnomalyComponent.h"
#include "Anomaly/ScaleAnomalyComponent.h"
#include "Runtime/Loop9ObservationIds.h"
#include "Runtime/Loop9RuntimePolicies.h"
#include "Subsystems/Loop9ObservationJournalSubsystem.h"
#include "Algo/Sort.h"
#include "Containers/Set.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	struct FCandidate
	{
		UAnomalyComponentBase* Component = nullptr;
		float Probability = 0.0f;
		float Weight = 1.0f;
	};

	constexpr bool bDebugOnlyMoveAnomaly = false;

	constexpr int32 AnomalyTypeCount = 12;

	constexpr ELoopAnomalyType AnomalyTypeOrder[AnomalyTypeCount] =
	{
		ELoopAnomalyType::Hide,
		ELoopAnomalyType::Move,
		ELoopAnomalyType::Light,
		ELoopAnomalyType::Audio,
		ELoopAnomalyType::Text,
		ELoopAnomalyType::DoorLock,
		ELoopAnomalyType::Pursuer,
		ELoopAnomalyType::Scale,
		ELoopAnomalyType::PhantomMessage,
		ELoopAnomalyType::LoopNumber,
		ELoopAnomalyType::Watcher,
		ELoopAnomalyType::Creep
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

	/**
	 * How often a type is drawn relative to the others, where 1.0 is the norm.
	 *
	 * Not every anomaly is equally good at being an anomaly. The floor is a
	 * search, so the types the player can actually solve by looking carry it,
	 * and the ones that resolve themselves stay garnish.
	 */
	float GetTypeSelectionWeight(ELoopAnomalyType Type)
	{
		switch (Type)
		{
		// A missing object and a repainted surface are the two fair anomalies:
		// the answer is in the room, and finding it feels like the player's own
		// work rather than something that happened at them.
		case ELoopAnomalyType::Hide:
		case ELoopAnomalyType::Text:
			return 1.8f;

		// The pursuer converts the floor from an inspection into a chase and
		// removes the choice at the elevator, so it lands as a rare shock.
		case ELoopAnomalyType::Pursuer:
			return 0.35f;

		// The loop counter sits on the wall in front of the player, so it is
		// easier to call than a missing stapler. Keep it garnish.
		case ELoopAnomalyType::LoopNumber:
			return 0.55f;

		// A man with his back turned is a shock, not a search. Rare like the
		// pursuer, a touch more common because he never takes the choice away.
		case ELoopAnomalyType::Watcher:
			return 0.45f;

		// A slow drift is a fair search once the player knows to compare, and a
		// blank until then. Keep it at the norm.
		case ELoopAnomalyType::Creep:
			return 1.0f;

		default:
			return 1.0f;
		}
	}

	/**
	 * Draws an index in proportion to Weights, or INDEX_NONE when nothing has
	 * positive weight. Walking the running sum keeps the draw allocation-free.
	 */
	int32 DrawWeightedIndex(const TArray<float>& Weights)
	{
		float TotalWeight = 0.0f;
		for (const float Weight : Weights)
		{
			TotalWeight += FMath::Max(Weight, 0.0f);
		}

		if (TotalWeight <= 0.0f)
		{
			return INDEX_NONE;
		}

		float Target = FMath::FRandRange(0.0f, TotalWeight);
		for (int32 Index = 0; Index < Weights.Num(); ++Index)
		{
			Target -= FMath::Max(Weights[Index], 0.0f);
			if (Target <= 0.0f)
			{
				return Index;
			}
		}

		return Weights.Num() - 1;
	}

	/** One pool per entry in AnomalyTypeOrder, holding every eligible component. */
	using FCandidatePools = TArray<TArray<FCandidate>>;

	FCandidatePools CollectCandidates(
		const TArray<TWeakObjectPtr<UAnomalyComponentBase>>& RegisteredComponents,
		float MinProbability)
	{
		FCandidatePools Pools;
		Pools.SetNum(AnomalyTypeCount);

		for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
		{
			UAnomalyComponentBase* Component = ComponentPtr.Get();
			if (!Component || Component->bIsAnomalyActive || Component->AnomalyProbability < MinProbability)
			{
				continue;
			}

			const ELoopAnomalyType Type = Component->GetAnomalyType();
			if (bDebugOnlyMoveAnomaly && Type != ELoopAnomalyType::Move)
			{
				continue;
			}

			const int32 TypeIndex = GetTypeIndex(Type);
			if (TypeIndex == INDEX_NONE)
			{
				continue;
			}

			FCandidate Candidate;
			Candidate.Component = Component;
			Candidate.Probability = Component->AnomalyProbability;
			Candidate.Weight = FMath::Max(Component->SelectionWeight, 0.01f);
			Pools[TypeIndex].Add(Candidate);
		}

		return Pools;
	}

	/**
	 * Draws one candidate and removes it from its pool, so a caller that keeps
	 * drawing walks the whole level exactly once. Returns false when nothing is
	 * left to draw.
	 */
	bool DrawCandidate(FCandidatePools& Pools, FCandidate& OutCandidate)
	{
		TArray<int32> AvailableTypeIndices;
		TArray<float> TypeWeights;
		for (int32 Index = 0; Index < AnomalyTypeCount; ++Index)
		{
			if (Pools[Index].Num() > 0)
			{
				AvailableTypeIndices.Add(Index);
				TypeWeights.Add(GetTypeSelectionWeight(AnomalyTypeOrder[Index]));
			}
		}

		const int32 DrawnTypeSlot = DrawWeightedIndex(TypeWeights);
		if (DrawnTypeSlot == INDEX_NONE)
		{
			return false;
		}

		TArray<FCandidate>& ChosenPool = Pools[AvailableTypeIndices[DrawnTypeSlot]];

		TArray<float> ComponentWeights;
		ComponentWeights.Reserve(ChosenPool.Num());
		for (const FCandidate& Candidate : ChosenPool)
		{
			ComponentWeights.Add(Candidate.Weight);
		}

		const int32 DrawnComponentIndex = DrawWeightedIndex(ComponentWeights);
		if (DrawnComponentIndex == INDEX_NONE)
		{
			ChosenPool.Empty();
			return DrawCandidate(Pools, OutCandidate);
		}

		OutCandidate = ChosenPool[DrawnComponentIndex];
		ChosenPool.RemoveAt(DrawnComponentIndex);
		return true;
	}
}

void UAnomalyManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAnomalyManager::Deinitialize()
{
	ResetRunTracking();
	Super::Deinitialize();
	RegisteredComponents.Empty();
}

void UAnomalyManager::BeginLoopVisit()
{
	// The judged floor was captured at decision time; keep its place for the
	// stale-floor slip before the new floor overwrites it.
	PreviousLoopAnomalyZone = CurrentLoopAnomalyZone;
	PreviousLoopAnomalyObjectKind = CurrentLoopAnomalyObjectKind;
	TrackedLoopIndex = INDEX_NONE;
	CurrentLoopAnomalyKey.Empty();
	CurrentLoopAnomalyContext = TEXT("No active anomaly currently detected.");
	CurrentLoopAnomalyZone.Empty();
	CurrentLoopAnomalyObjectKind.Empty();
	bCurrentLoopAnomalyRepeat = false;
	bPhoneLineCutThisFloor = false;
}

void UAnomalyManager::ResetRunTracking()
{
	BeginLoopVisit();
	PreviousLoopAnomalyKey.Empty();
	PreviousLoopAnomalyZone.Empty();
	PreviousLoopAnomalyObjectKind.Empty();
}

void UAnomalyManager::CleanupInvalidComponents()
{
	RegisteredComponents.RemoveAll([](const TWeakObjectPtr<UAnomalyComponentBase>& Component)
	{
		return !Component.IsValid();
	});
}

void UAnomalyManager::EnsureScaleAnomalyPlacement(UWorld* World)
{
	if (!World)
	{
		return;
	}

	CleanupInvalidComponents();
	for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
	{
		if (const UAnomalyComponentBase* Component = ComponentPtr.Get())
		{
			if (Component->GetAnomalyType() == ELoopAnomalyType::Scale)
			{
				return;
			}
		}
	}

	AActor* Host = nullptr;
	AActor* Fallback = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		const FString Label = Actor->GetActorNameOrLabel();
		if (Label.Contains(TEXT("SM_ComputerPrinter_A01_N1")))
		{
			Host = Actor;
			break;
		}
		if (!Fallback && Label.Contains(TEXT("ComputerPrinter")))
		{
			Fallback = Actor;
		}
	}

	if (!Host)
	{
		Host = Fallback;
	}

	if (!Host)
	{
		UE_LOG(LogLoop9, Warning,
			TEXT("AnomalyManager: ScaleAnomaly is not in the map and no ComputerPrinter host was found in '%s'"),
			*World->GetMapName());
		return;
	}

	UScaleAnomalyComponent* Scale = NewObject<UScaleAnomalyComponent>(Host, TEXT("ScaleAnomaly"));
	Scale->AnomalyZone = TEXT("the back shelves past the lifts");
	Scale->AnomalyObjectKind = TEXT("a printer");
	Scale->ScaleMultiplier = 1.4f;
	Host->AddInstanceComponent(Scale);
	Scale->RegisterComponent();

	UE_LOG(LogLoop9, Log, TEXT("AnomalyManager: attached ScaleAnomaly to '%s' (map had none)"),
		*Host->GetActorNameOrLabel());
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
	FString NewZone;
	FString NewObjectKind;
	ComputeActiveAnomalySnapshot(NewKey, NewContext, NewZone, NewObjectKind);

	bCurrentLoopAnomalyRepeat = !PreviousLoopAnomalyKey.IsEmpty() && PreviousLoopAnomalyKey == NewKey;
	CurrentLoopAnomalyKey = NewKey;
	CurrentLoopAnomalyContext = NewContext;
	CurrentLoopAnomalyZone = NewZone;
	CurrentLoopAnomalyObjectKind = NewObjectKind;
	PreviousLoopAnomalyKey = NewKey;
	TrackedLoopIndex = LoopIndex;
}

void UAnomalyManager::ComputeActiveAnomalySnapshot(
	FString& OutKey,
	FString& OutContext,
	FString& OutZone,
	FString& OutObjectKind) const
{
	TArray<FString> Labels;
	TArray<Loop9RuntimePolicies::FAnomalyDetailCandidate> DetailCandidates;

	for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
	{
		const UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (!Component || !Component->bIsAnomalyActive)
		{
			continue;
		}

		Labels.Add(Component->GetAnomalyTypeLabel().ToString());

		Loop9RuntimePolicies::FAnomalyDetailCandidate Candidate;
		Candidate.TypePriority = GetTypeIndex(Component->GetAnomalyType());
		Candidate.Zone = Component->AnomalyZone;
		Candidate.ObjectKind = Component->AnomalyObjectKind;
		DetailCandidates.Add(Candidate);
	}

	const Loop9RuntimePolicies::FAnomalyDetailCandidate Detail =
		Loop9RuntimePolicies::SelectAnomalyDetail(DetailCandidates);
	OutZone = Detail.Zone;
	OutObjectKind = Detail.ObjectKind;

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

FString UAnomalyManager::SelectDecoyZone() const
{
	TArray<FString> ActiveZones;
	TArray<FString> InactiveAuthoredZones;
	const ULoop9ObservationJournalSubsystem* Journal = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULoop9ObservationJournalSubsystem>()
		: nullptr;

	for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
	{
		const UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (!Component)
		{
			continue;
		}

		const FString Zone = Component->AnomalyZone.TrimStartAndEnd();
		if (Zone.IsEmpty())
		{
			continue;
		}

		if (Component->bIsAnomalyActive)
		{
			ActiveZones.AddUnique(Zone);
			continue;
		}

		const ELoopAnomalyType Type = Component->GetAnomalyType();
		if (Type == ELoopAnomalyType::Pursuer || Type == ELoopAnomalyType::PhantomMessage)
		{
			continue;
		}

		const FName ZoneId = Loop9ObservationIds::Canonicalize(Zone);
		if (Journal
			&& Journal->IsZoneRegistered(ZoneId)
			&& !Journal->IsPlayerInsideZone(ZoneId))
		{
			InactiveAuthoredZones.Add(Zone);
		}
	}

	return Loop9RuntimePolicies::SelectDecoyZone(InactiveAuthoredZones, ActiveZones);
}

bool UAnomalyManager::ForceActivateAnyAnomaly()
{
	CleanupInvalidComponents();

	// This is the guarantee behind "the floor said it has an anomaly", so it
	// ignores AnomalyProbability but keeps the type weights: a rescued floor
	// should not quietly become the one place the pursuer is common. It also
	// keeps drawing past a component that refuses to apply, otherwise a single
	// misconfigured placement could hand the player a silently clean floor.
	FCandidatePools CandidatePools = CollectCandidates(RegisteredComponents, /*MinProbability*/ 0.0f);

	FCandidate Selected;
	while (DrawCandidate(CandidatePools, Selected))
	{
		if (ForceActivateComponent(Selected.Component, INDEX_NONE))
		{
			return true;
		}
	}

	return false;
}

bool UAnomalyManager::ForceActivateByFilter(const FString& Filter, int32 MaterialIndex)
{
#if UE_BUILD_SHIPPING
	(void)Filter;
	(void)MaterialIndex;
	return false;
#else
	CleanupInvalidComponents();

	const FString NormalizedFilter = Filter.TrimStartAndEnd();
	if (NormalizedFilter.IsEmpty())
	{
		UE_LOG(LogLoop9, Warning, TEXT("AnomalyManager: empty filter"));
		return false;
	}

	TArray<UAnomalyComponentBase*> Matches;
	for (TWeakObjectPtr<UAnomalyComponentBase> ComponentPtr : RegisteredComponents)
	{
		UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (Component && DoesComponentMatchFilter(Component, NormalizedFilter))
		{
			Matches.Add(Component);
		}
	}

	if (Matches.Num() == 0)
	{
		UE_LOG(LogLoop9, Warning, TEXT("AnomalyManager: no anomalies matching filter '%s'"), *NormalizedFilter);
		return false;
	}

	int32 Activated = 0;
	for (UAnomalyComponentBase* Chosen : Matches)
	{
		if (ForceActivateComponent(Chosen, MaterialIndex))
		{
			++Activated;
			const AActor* Owner = Chosen->GetOwner();

			UE_LOG(LogLoop9, Log, TEXT("AnomalyManager: forced '%s' on '%s'"),
				*Chosen->GetClass()->GetName(),
				Owner ? *Owner->GetActorNameOrLabel() : TEXT("None"));
		}
	}

	UE_LOG(LogLoop9, Log, TEXT("AnomalyManager: filter '%s' activated %d / %d"), *NormalizedFilter, Activated, Matches.Num());
	return Activated > 0;
#endif
}

bool UAnomalyManager::DoesComponentMatchFilter(const UAnomalyComponentBase* Component, const FString& Filter)
{
	if (!Component)
	{
		return false;
	}

	const FString TypeLabel = Component->GetAnomalyTypeLabel().ToString();
	if (TypeLabel.Equals(Filter, ESearchCase::IgnoreCase))
	{
		return true;
	}

	// Allow short type names: Text, Move, Hide, Light Flicker, ...
	const FString TypeName = UEnum::GetDisplayValueAsText(Component->GetAnomalyType()).ToString();
	if (TypeName.Equals(Filter, ESearchCase::IgnoreCase))
	{
		return true;
	}

	const FString TypeNameCompact = TypeName.Replace(TEXT(" "), TEXT(""));
	if (TypeNameCompact.Equals(Filter, ESearchCase::IgnoreCase))
	{
		return true;
	}

	if (Filter.Equals(TEXT("Flicker"), ESearchCase::IgnoreCase)
		&& Component->GetAnomalyType() == ELoopAnomalyType::Light)
	{
		return true;
	}
	if ((Filter.Equals(TEXT("Phone"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("Telephone"), ESearchCase::IgnoreCase))
		&& Component->GetAnomalyType() == ELoopAnomalyType::Audio)
	{
		return true;
	}
	if ((Filter.Equals(TEXT("Door"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("Doors"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("DoorLock"), ESearchCase::IgnoreCase))
		&& Component->GetAnomalyType() == ELoopAnomalyType::DoorLock)
	{
		return true;
	}
	if ((Filter.Equals(TEXT("LoopNumber"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("Counter"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("QuestionMark"), ESearchCase::IgnoreCase))
		&& Component->GetAnomalyType() == ELoopAnomalyType::LoopNumber)
	{
		return true;
	}
	if ((Filter.Equals(TEXT("Phantom"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("PhantomMessage"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("PhantomMsg"), ESearchCase::IgnoreCase))
		&& Component->GetAnomalyType() == ELoopAnomalyType::PhantomMessage)
	{
		return true;
	}
	if (Filter.Equals(TEXT("Scale"), ESearchCase::IgnoreCase)
		&& Component->GetAnomalyType() == ELoopAnomalyType::Scale)
	{
		return true;
	}
	if ((Filter.Equals(TEXT("Watcher"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("Figure"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("BackTurned"), ESearchCase::IgnoreCase))
		&& Component->GetAnomalyType() == ELoopAnomalyType::Watcher)
	{
		return true;
	}
	if ((Filter.Equals(TEXT("Creep"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("Drift"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("SlowMove"), ESearchCase::IgnoreCase))
		&& Component->GetAnomalyType() == ELoopAnomalyType::Creep)
	{
		return true;
	}
	if ((Filter.Equals(TEXT("Hide"), ESearchCase::IgnoreCase)
			|| Filter.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase))
		&& Component->GetAnomalyType() == ELoopAnomalyType::Hide)
	{
		return true;
	}

	// Avoid accidental mass activation from filters such as "a", "e" or "01".
	if (Filter.Len() < 3)
	{
		return false;
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

bool UAnomalyManager::ForceActivateComponent(UAnomalyComponentBase* Component, int32 MaterialIndex)
{
	if (!Component)
	{
		return false;
	}

	if (UMaterialSwapAnomalyComponent* MaterialSwap = Cast<UMaterialSwapAnomalyComponent>(Component))
	{
		if (MaterialIndex >= 0)
		{
			return MaterialSwap->ForceMaterialVariant(MaterialIndex);
		}
	}

	if (Component->bIsAnomalyActive)
	{
		return true;
	}

	Component->ActivateAnomaly();
	return Component->bIsAnomalyActive;
}

void UAnomalyManager::TriggerRandomAnomalies(int32 Count, float MinProbability)
{
	CleanupInvalidComponents();

	if (RegisteredComponents.Num() == 0)
	{
		return;
	}

	FCandidatePools CandidatePools = CollectCandidates(RegisteredComponents, MinProbability);

	// Drawing removes the candidate, so this walks each placement at most once
	// and stops when the floor is full or the level has nothing left to offer.
	// A component that loses its probability roll, or whose ApplyAnomalyState
	// refuses, therefore no longer costs the floor an anomaly: the next draw
	// takes its place.
	int32 AnomaliesTriggered = 0;
	FCandidate Selected;
	while (AnomaliesTriggered < Count && DrawCandidate(CandidatePools, Selected))
	{
		if (!Selected.Component || FMath::FRand() > Selected.Probability)
		{
			continue;
		}

		Selected.Component->ActivateAnomaly();
		if (Selected.Component->bIsAnomalyActive)
		{
			AnomaliesTriggered++;
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

bool UAnomalyManager::IsAnomalyTypeActive(ELoopAnomalyType Type) const
{
	for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
	{
		const UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (Component && Component->bIsAnomalyActive && Component->GetAnomalyType() == Type)
		{
			return true;
		}
	}

	return false;
}

bool UAnomalyManager::IsAnomalyTypeActiveOn(ELoopAnomalyType Type, const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
	{
		const UAnomalyComponentBase* Component = ComponentPtr.Get();
		if (Component
			&& Component->bIsAnomalyActive
			&& Component->GetAnomalyType() == Type
			&& Component->GetOwner() == Actor)
		{
			return true;
		}
	}

	return false;
}

void UAnomalyManager::PrintAnomalyStats()
{
#if !UE_BUILD_SHIPPING
	CleanupInvalidComponents();

	UE_LOG(LogLoop9, Log, TEXT("===== Anomaly Stats (%d registered, %d active) ====="),
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

		UE_LOG(LogLoop9, Log, TEXT("[%d] %s | %s | %s | active=%d | p=%.2f%s"),
			Index++,
			Owner ? *Owner->GetActorNameOrLabel() : TEXT("None"),
			*Component->GetAnomalyTypeLabel().ToString(),
			*Component->GetClass()->GetName(),
			Component->bIsAnomalyActive ? 1 : 0,
			Component->AnomalyProbability,
			*Extra);
	}

	UE_LOG(LogLoop9, Log, TEXT("Commands: AnomalyList | AnomalyReset | AnomalyForce <filter> [matIndex] | AnomalyForceAny | AnomalyAuditMaterials"));
#endif
}

int32 UAnomalyManager::AuditMaterialAnomalies()
{
	CleanupInvalidComponents();

	int32 Checked = 0;
	int32 Broken = 0;

	for (const TWeakObjectPtr<UAnomalyComponentBase>& ComponentPtr : RegisteredComponents)
	{
		UMaterialSwapAnomalyComponent* MaterialSwap = Cast<UMaterialSwapAnomalyComponent>(ComponentPtr.Get());
		if (!MaterialSwap)
		{
			continue;
		}

		++Checked;
		const AActor* Owner = MaterialSwap->GetOwner();
		const FString OwnerName = Owner ? Owner->GetActorNameOrLabel() : TEXT("None");
		const FString Problem = MaterialSwap->DescribeConfigurationProblem();

		if (Problem.IsEmpty())
		{
			UE_LOG(LogLoop9, Log, TEXT("MaterialAudit: OK   '%s' (slot %d, %d variants)"),
				*OwnerName, MaterialSwap->MaterialSlot, MaterialSwap->AnomalyMaterials.Num());
		}
		else
		{
			++Broken;
			UE_LOG(LogLoop9, Warning, TEXT("MaterialAudit: FAIL '%s' — %s"), *OwnerName, *Problem);
		}
	}

	UE_LOG(LogLoop9, Log, TEXT("MaterialAudit: %d material anomalies checked, %d need authoring"), Checked, Broken);
	return Broken;
}
