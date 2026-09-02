#pragma once

#include "CoreMinimal.h"
#include "Runtime/Loop9ObservationJournal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop9ObservationJournalSubsystem.generated.h"

class ALoop9ObservationZoneVolume;

DECLARE_MULTICAST_DELEGATE_OneParam(FLoop9ZoneEnteredDelegate, FName);

/**
 * Owns the bounded run/floor observation journal and the authored zone
 * registry. This context is advisory input only and never drives gameplay.
 */
UCLASS()
class LOOP9_API ULoop9ObservationJournalSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void BeginFloor(int32 FloorIndex);
	void ResetRun();
	void RecordObjectInspected(FName SubjectId = NAME_None);
	void RecordDoorOpened(FName SubjectId = NAME_None);
	void RecordDoorClosed(FName SubjectId = NAME_None);
	void RecordDoorDenied(FName SubjectId = NAME_None);
	void RecordFlashlightState(bool bEnabled);
	void RecordPursuerObserved();
	void RecordPursuerCaught();
	void RecordAIInteraction();
	void RecordElevatorDecision(FName ChoiceId, bool bWasCorrect);
	FLoop9ObservationSnapshot GetSnapshot() const;

	void RegisterZoneVolume(ALoop9ObservationZoneVolume* Volume, FName ZoneId);
	void UnregisterZoneVolume(ALoop9ObservationZoneVolume* Volume);
	void NotifyPlayerEnteredZone(ALoop9ObservationZoneVolume* Volume);
	void NotifyPlayerExitedZone(ALoop9ObservationZoneVolume* Volume);
	bool IsZoneRegistered(FName ZoneId) const;
	bool IsPlayerInsideZone(FName ZoneId) const;

	FLoop9ZoneEnteredDelegate OnZoneEntered;

private:
	struct FRegisteredZone
	{
		TWeakObjectPtr<ALoop9ObservationZoneVolume> Volume;
		FName ZoneId = NAME_None;
	};

	void RecordEvent(
		ELoop9ObservationEventType Type,
		FName SubjectId = NAME_None,
		FName ZoneOverride = NAME_None);
	FName FindZoneId(const ALoop9ObservationZoneVolume* Volume) const;
	void RefreshCurrentZone(bool bRememberVisit = true);
	void PruneZoneRegistry();
	void ReconcileOccupiedVolumes();

	FLoop9ObservationJournalCore Journal;
	TArray<FRegisteredZone> RegisteredZones;
	TArray<TWeakObjectPtr<ALoop9ObservationZoneVolume>> OccupiedVolumes;
};
