#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Runtime/Loop9ObservationJournal.h"

/** Backend wire adapter for privacy-safe observation snapshots. */
class LOOP9_API FLoop9ObservationCodec
{
public:
	static FString EventTypeToWire(ELoop9ObservationEventType Type);

	/**
	 * Builds a snapshot object and deterministically drops trailing projected
	 * events until its condensed UTF-8 representation fits the budget.
	 */
	static TSharedPtr<FJsonObject> BuildSnapshotObject(
		const FLoop9ObservationSnapshot& Snapshot,
		int32 MaxUtf8Bytes = 1024);

	/** Condensed representation used for budget verification and tests. */
	static FString SerializeSnapshot(
		const FLoop9ObservationSnapshot& Snapshot,
		int32 MaxUtf8Bytes = 1024);

private:
	static FString SerializeObject(const TSharedRef<FJsonObject>& Object);
};
