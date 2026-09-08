#include "Subsystems/Loop9DragojloMemorySubsystem.h"

#include "Subsystems/Loop9AchievementsSubsystem.h"

void ULoop9DragojloMemorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// The achievements subsystem carries the Cloud file; make sure it exists first.
	Collection.InitializeDependency<ULoop9AchievementsSubsystem>();

	if (const ULoop9AchievementsSubsystem* Store = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
	{
		Memory = FDragojloMemory::FromPersistedString(Store->LoadDragojloMemory());
	}

	UE_LOG(LogTemp, Log, TEXT("Dragojlo memory loaded: runs=%d lies=%d caught=%d"),
		Memory.RunsFinished, Memory.LiesTold, Memory.CaughtLying);
}

void ULoop9DragojloMemorySubsystem::RecordRunFinished(
	ELoopEndingType EndingType,
	int32 TotalAIInteractions,
	float FinalKindness,
	const FDragojloCommitmentState& Commitment)
{
	Memory.RecordRunFinished(EndingType, TotalAIInteractions, FinalKindness, Commitment);
	Persist();
}

TOptional<FDragojloMemory> ULoop9DragojloMemorySubsystem::MemoryForChatRequest(int32 InteractionsThisRun) const
{
	if (!FDragojloMemory::ShouldSendToBackend(Memory, InteractionsThisRun))
	{
		return TOptional<FDragojloMemory>();
	}
	return Memory;
}

void ULoop9DragojloMemorySubsystem::ForgetEverything()
{
	Memory = FDragojloMemory();
	Persist();
}

void ULoop9DragojloMemorySubsystem::Persist() const
{
	if (const ULoop9AchievementsSubsystem* Store = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
	{
		Store->SaveDragojloMemory(Memory.ToPersistedString());
	}
}
