#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Runtime/DragojloMemory.h"
#include "Loop9DragojloMemorySubsystem.generated.h"

/**
 * Owns what Dragojlo remembers about the player between runs.
 *
 * Loaded once from the shared Game.ini / Steam Cloud record, folded on every
 * finished run, and offered to AAI_Friend for the first few phone replies of
 * a fresh run. It is tone only: nothing here is read by the ending evaluator,
 * the anomaly manager, achievements or telemetry, and ResetRunState leaves it
 * alone on purpose — a new shift does not make him forget you.
 */
UCLASS()
class LOOP9_API ULoop9DragojloMemorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FDragojloMemory& GetMemory() const { return Memory; }

	/** Called by the ending presenter once the ending is final. Persists immediately. */
	void RecordRunFinished(
		ELoopEndingType EndingType,
		int32 TotalAIInteractions,
		float FinalKindness,
		const FDragojloCommitmentState& Commitment);

	/** Memory to attach to a chat request, or unset when it should stay silent. */
	TOptional<FDragojloMemory> MemoryForChatRequest(int32 InteractionsThisRun) const;

	/** Debug / support only: wipes the record. Not exposed in the UI by design. */
	UFUNCTION(BlueprintCallable, Category = "Dragojlo|Debug")
	void ForgetEverything();

private:
	void Persist() const;

	FDragojloMemory Memory;
};
