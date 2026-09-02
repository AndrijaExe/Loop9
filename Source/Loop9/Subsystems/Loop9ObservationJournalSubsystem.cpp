#include "Subsystems/Loop9ObservationJournalSubsystem.h"

#include "HAL/PlatformTime.h"
#include "Interaction/Loop9ObservationZoneVolume.h"

void ULoop9ObservationJournalSubsystem::BeginFloor(int32 FloorIndex)
{
	Journal.BeginFloor(FloorIndex, FPlatformTime::Seconds());
	OccupiedVolumes.Reset();
}

void ULoop9ObservationJournalSubsystem::ResetRun()
{
	Journal.ResetRun();
	OccupiedVolumes.Reset();
}

void ULoop9ObservationJournalSubsystem::RecordEvent(
	ELoop9ObservationEventType Type,
	FName SubjectId,
	FName ZoneOverride)
{
	const FName ZoneId = ZoneOverride.IsNone() ? Journal.GetCurrentZone() : ZoneOverride;
	Journal.Record(Type, ZoneId, SubjectId, FPlatformTime::Seconds());
}

void ULoop9ObservationJournalSubsystem::RecordAIInteraction()
{
	Journal.RecordAIInteraction(FPlatformTime::Seconds());
}

void ULoop9ObservationJournalSubsystem::RecordElevatorDecision(FName ChoiceId, bool bWasCorrect)
{
	Journal.RecordElevatorDecision(ChoiceId, bWasCorrect, FPlatformTime::Seconds());
}

FLoop9ObservationSnapshot ULoop9ObservationJournalSubsystem::GetSnapshot() const
{
	return Journal.BuildSnapshot(FPlatformTime::Seconds());
}

void ULoop9ObservationJournalSubsystem::RegisterZoneVolume(
	ALoop9ObservationZoneVolume* Volume,
	FName ZoneId)
{
	if (!Volume)
	{
		return;
	}

	const FName SafeZoneId = FLoop9ObservationJournalCore::SanitizeIdentifier(ZoneId);
	if (SafeZoneId.IsNone())
	{
		return;
	}

	PruneZoneRegistry();
	for (FRegisteredZone& Registered : RegisteredZones)
	{
		if (Registered.Volume.Get() == Volume)
		{
			Registered.ZoneId = SafeZoneId;
			return;
		}
	}

	FRegisteredZone& Registered = RegisteredZones.AddDefaulted_GetRef();
	Registered.Volume = Volume;
	Registered.ZoneId = SafeZoneId;
}

void ULoop9ObservationJournalSubsystem::UnregisterZoneVolume(ALoop9ObservationZoneVolume* Volume)
{
	if (!Volume)
	{
		return;
	}
	RegisteredZones.RemoveAll([Volume](const FRegisteredZone& Registered)
	{
		return !Registered.Volume.IsValid() || Registered.Volume.Get() == Volume;
	});
	OccupiedVolumes.RemoveAll([Volume](const TWeakObjectPtr<ALoop9ObservationZoneVolume>& Occupied)
	{
		return !Occupied.IsValid() || Occupied.Get() == Volume;
	});
	RefreshCurrentZone();
}

void ULoop9ObservationJournalSubsystem::NotifyPlayerEnteredZone(ALoop9ObservationZoneVolume* Volume)
{
	const FName ZoneId = FindZoneId(Volume);
	if (ZoneId.IsNone())
	{
		return;
	}

	const bool bAlreadyOccupied = OccupiedVolumes.ContainsByPredicate(
		[Volume](const TWeakObjectPtr<ALoop9ObservationZoneVolume>& Occupied)
		{
			return Occupied.IsValid() && Occupied.Get() == Volume;
		});
	if (bAlreadyOccupied)
	{
		return;
	}

	OccupiedVolumes.RemoveAll([Volume](const TWeakObjectPtr<ALoop9ObservationZoneVolume>& Occupied)
	{
		return !Occupied.IsValid() || Occupied.Get() == Volume;
	});
	OccupiedVolumes.Add(Volume);
	Journal.Record(
		ELoop9ObservationEventType::ZoneEntered,
		ZoneId,
		NAME_None,
		FPlatformTime::Seconds());
	OnZoneEntered.Broadcast(ZoneId);
}

void ULoop9ObservationJournalSubsystem::NotifyPlayerExitedZone(ALoop9ObservationZoneVolume* Volume)
{
	OccupiedVolumes.RemoveAll([Volume](const TWeakObjectPtr<ALoop9ObservationZoneVolume>& Occupied)
	{
		return !Occupied.IsValid() || Occupied.Get() == Volume;
	});
	RefreshCurrentZone();
}

bool ULoop9ObservationJournalSubsystem::IsZoneRegistered(FName ZoneId) const
{
	const FName SafeZoneId = FLoop9ObservationJournalCore::SanitizeIdentifier(ZoneId);
	for (const FRegisteredZone& Registered : RegisteredZones)
	{
		if (Registered.Volume.IsValid() && Registered.ZoneId == SafeZoneId)
		{
			return true;
		}
	}
	return false;
}

bool ULoop9ObservationJournalSubsystem::IsPlayerInsideZone(FName ZoneId) const
{
	const FName SafeZoneId = FLoop9ObservationJournalCore::SanitizeIdentifier(ZoneId);
	for (const TWeakObjectPtr<ALoop9ObservationZoneVolume>& Occupied : OccupiedVolumes)
	{
		if (Occupied.IsValid() && FindZoneId(Occupied.Get()) == SafeZoneId)
		{
			return true;
		}
	}
	return false;
}

FName ULoop9ObservationJournalSubsystem::FindZoneId(
	const ALoop9ObservationZoneVolume* Volume) const
{
	for (const FRegisteredZone& Registered : RegisteredZones)
	{
		if (Registered.Volume.Get() == Volume)
		{
			return Registered.ZoneId;
		}
	}
	return NAME_None;
}

void ULoop9ObservationJournalSubsystem::RefreshCurrentZone()
{
	OccupiedVolumes.RemoveAll([](const TWeakObjectPtr<ALoop9ObservationZoneVolume>& Volume)
	{
		return !Volume.IsValid();
	});

	if (OccupiedVolumes.IsEmpty())
	{
		Journal.ClearCurrentZone(Journal.GetCurrentZone());
		return;
	}

	Journal.SetCurrentZone(FindZoneId(OccupiedVolumes.Last().Get()));
}

void ULoop9ObservationJournalSubsystem::PruneZoneRegistry()
{
	RegisteredZones.RemoveAll([](const FRegisteredZone& Registered)
	{
		return !Registered.Volume.IsValid();
	});
	OccupiedVolumes.RemoveAll([](const TWeakObjectPtr<ALoop9ObservationZoneVolume>& Volume)
	{
		return !Volume.IsValid();
	});
}
