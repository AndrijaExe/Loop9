#include "Subsystems/Loop9ObservationJournalSubsystem.h"

#include "HAL/PlatformTime.h"
#include "GameFramework/Pawn.h"
#include "Interaction/Loop9ObservationZoneVolume.h"
#include "Kismet/GameplayStatics.h"

void ULoop9ObservationJournalSubsystem::BeginFloor(int32 FloorIndex)
{
	PruneZoneRegistry();
	Journal.BeginFloor(FloorIndex, FPlatformTime::Seconds());
	// Physical overlap can span a floor rollover (notably during the elevator
	// cinematic). Restore the current zone without inventing a new entered event.
	ReconcileOccupiedVolumes();
	RefreshCurrentZone(false);
}

void ULoop9ObservationJournalSubsystem::ResetRun()
{
	Journal.ResetRun();
	OccupiedVolumes.Reset();
}

void ULoop9ObservationJournalSubsystem::RecordObjectInspected(FName SubjectId)
{
	RecordEvent(
		ELoop9ObservationEventType::ObjectInspected,
		SubjectId.IsNone() ? FName(TEXT("generic_object")) : SubjectId);
}

void ULoop9ObservationJournalSubsystem::RecordDoorOpened(FName SubjectId)
{
	RecordEvent(
		ELoop9ObservationEventType::DoorOpened,
		SubjectId.IsNone() ? FName(TEXT("generic_door")) : SubjectId);
}

void ULoop9ObservationJournalSubsystem::RecordDoorClosed(FName SubjectId)
{
	RecordEvent(
		ELoop9ObservationEventType::DoorClosed,
		SubjectId.IsNone() ? FName(TEXT("generic_door")) : SubjectId);
}

void ULoop9ObservationJournalSubsystem::RecordDoorDenied(FName SubjectId)
{
	RecordEvent(
		ELoop9ObservationEventType::DoorDenied,
		SubjectId.IsNone() ? FName(TEXT("generic_door")) : SubjectId);
}

void ULoop9ObservationJournalSubsystem::RecordFlashlightState(bool bEnabled)
{
	RecordEvent(
		bEnabled
			? ELoop9ObservationEventType::FlashlightOn
			: ELoop9ObservationEventType::FlashlightOff);
}

void ULoop9ObservationJournalSubsystem::RecordPursuerObserved()
{
	RecordEvent(
		ELoop9ObservationEventType::PursuerObserved,
		FName(TEXT("generic_pursuer")));
}

void ULoop9ObservationJournalSubsystem::RecordPursuerCaught()
{
	RecordEvent(
		ELoop9ObservationEventType::PursuerCaught,
		FName(TEXT("generic_pursuer")));
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

	const FName SafeZoneId = Loop9ObservationIds::Canonicalize(ZoneId);
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
	const FName SafeZoneId = Loop9ObservationIds::Canonicalize(ZoneId);
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
	const FName SafeZoneId = Loop9ObservationIds::Canonicalize(ZoneId);
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

void ULoop9ObservationJournalSubsystem::RefreshCurrentZone(bool bRememberVisit)
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

	const FName ZoneId = FindZoneId(OccupiedVolumes.Last().Get());
	if (bRememberVisit)
	{
		Journal.SetCurrentZone(ZoneId);
	}
	else
	{
		Journal.RestoreCurrentZone(ZoneId);
	}
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

void ULoop9ObservationJournalSubsystem::ReconcileOccupiedVolumes()
{
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	OccupiedVolumes.RemoveAll([PlayerPawn](const TWeakObjectPtr<ALoop9ObservationZoneVolume>& Volume)
	{
		return !Volume.IsValid() || !Volume->IsOverlappingActor(PlayerPawn);
	});

	for (const FRegisteredZone& Registered : RegisteredZones)
	{
		ALoop9ObservationZoneVolume* Volume = Registered.Volume.Get();
		if (Volume
			&& Volume->IsOverlappingActor(PlayerPawn)
			&& !OccupiedVolumes.Contains(Volume))
		{
			OccupiedVolumes.Add(Volume);
		}
	}
}
