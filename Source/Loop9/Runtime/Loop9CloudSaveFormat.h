#pragma once

#include "CoreMinimal.h"

class FConfigFile;

/**
 * Layout of the one Game.ini section Steam Auto-Cloud carries between
 * machines: achievements progress and Dragojlo's cross-run memory. The file is
 * never edited in place — it is rebuilt from PersistedKeys() on every save, so
 * a value the writer does not know about is lost on the next save. Adding a
 * key means adding it here; Loop9.Runtime.CloudSave.KeepsEveryKey checks the
 * list round-trips.
 */
namespace Loop9CloudSaveFormat
{
	inline constexpr const TCHAR* Section = TEXT("/Script/Loop9.Loop9AchievementsSubsystem");
	inline constexpr const TCHAR* CloudReadyKey = TEXT("CloudReady");
	inline constexpr const TCHAR* SeenEndingsKey = TEXT("SeenEndings");
	inline constexpr const TCHAR* SpottedAnomaliesKey = TEXT("SpottedAnomalies");
	inline constexpr const TCHAR* PendingUnlocksKey = TEXT("PendingUnlocks");
	/** Cross-run Dragojlo memory (see FDragojloMemory). */
	inline constexpr const TCHAR* DragojloMemoryKey = TEXT("DragojloMemory");

	/** Every value the file keeps, in file order. */
	LOOP9_API TArrayView<const TCHAR* const> PersistedKeys();

	/** True when Key names one of PersistedKeys(). */
	LOOP9_API bool IsPersistedKey(const TCHAR* Key);

	/**
	 * Full file text: the section header, CloudReady=1, then every persisted
	 * key read from File (an empty value when the file does not have it yet).
	 */
	LOOP9_API FString BuildIniText(const FConfigFile& File);
}
