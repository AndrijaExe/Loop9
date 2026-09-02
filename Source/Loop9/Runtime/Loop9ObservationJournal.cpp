#include "Runtime/Loop9ObservationJournal.h"

#include "Algo/Reverse.h"

void FLoop9ObservationJournalCore::ResetRun()
{
	Events.Reset();
	VisitedZones.Reset();
	RunSummary = FLoop9ObservationRunSummary();
	CurrentZone = NAME_None;
	CurrentFloorIndex = 0;
	FloorStartedAt = 0.0;
	NextSequence = 0;
}

void FLoop9ObservationJournalCore::BeginFloor(int32 FloorIndex, double NowSeconds)
{
	Events.Reset();
	VisitedZones.Reset();
	CurrentZone = NAME_None;
	CurrentFloorIndex = FMath::Max(0, FloorIndex);
	FloorStartedAt = FMath::Max(0.0, NowSeconds);
	NextSequence = 0;
	RunSummary.FloorsStarted = SaturatingIncrement(RunSummary.FloorsStarted);
}

void FLoop9ObservationJournalCore::SetCurrentZone(FName ZoneId)
{
	CurrentZone = Loop9ObservationIds::Canonicalize(ZoneId);
	RememberVisitedZone(CurrentZone);
}

void FLoop9ObservationJournalCore::RestoreCurrentZone(FName ZoneId)
{
	CurrentZone = Loop9ObservationIds::Canonicalize(ZoneId);
}

void FLoop9ObservationJournalCore::ClearCurrentZone(FName ZoneId)
{
	if (CurrentZone == Loop9ObservationIds::Canonicalize(ZoneId))
	{
		CurrentZone = NAME_None;
	}
}

void FLoop9ObservationJournalCore::Record(
	ELoop9ObservationEventType Type,
	FName ZoneId,
	FName SubjectId,
	double NowSeconds)
{
	const FName SafeZone = Loop9ObservationIds::Canonicalize(ZoneId);
	const FName SafeSubject = Loop9ObservationIds::Canonicalize(SubjectId);
	const uint16 AtSecond = SaturatingSeconds(NowSeconds - FloorStartedAt);

	if (Type == ELoop9ObservationEventType::ZoneEntered)
	{
		SetCurrentZone(SafeZone);
	}

	for (FLoop9ObservationEvent& Event : Events)
	{
		if (Event.Type == Type && Event.ZoneId == SafeZone && Event.SubjectId == SafeSubject)
		{
			Event.Count = Event.Count == MAX_uint8 ? MAX_uint8 : Event.Count + 1;
			Event.AtSecond = AtSecond;
			Event.Sequence = ++NextSequence;
			return;
		}
	}

	FLoop9ObservationEvent& Event = Events.AddDefaulted_GetRef();
	Event.Type = Type;
	Event.ZoneId = SafeZone;
	Event.SubjectId = SafeSubject;
	Event.Count = 1;
	Event.AtSecond = AtSecond;
	Event.Sequence = ++NextSequence;
	EnforceEventCap();
}

void FLoop9ObservationJournalCore::RecordAIInteraction(double NowSeconds)
{
	RunSummary.AIInteractions = SaturatingIncrement(RunSummary.AIInteractions);
	Record(ELoop9ObservationEventType::CallCompleted, CurrentZone, NAME_None, NowSeconds);
}

void FLoop9ObservationJournalCore::RecordElevatorDecision(
	FName ChoiceId,
	bool bWasCorrect,
	double NowSeconds)
{
	RunSummary.ElevatorDecisions = SaturatingIncrement(RunSummary.ElevatorDecisions);
	if (bWasCorrect)
	{
		RunSummary.CorrectDecisions = SaturatingIncrement(RunSummary.CorrectDecisions);
	}
	// The floor journal is reset immediately after a committed lift decision,
	// so the ordered event would never reach a later AI request. Keep this
	// information only in the fixed run summary.
	(void)ChoiceId;
	(void)NowSeconds;
}

FLoop9ObservationSnapshot FLoop9ObservationJournalCore::BuildSnapshot(double NowSeconds) const
{
	FLoop9ObservationSnapshot Snapshot;
	Snapshot.CurrentZone = CurrentZone;
	Snapshot.SecondsOnFloor = SaturatingSeconds(NowSeconds - FloorStartedAt);
	Snapshot.VisitedZones = VisitedZones;
	Algo::Reverse(Snapshot.VisitedZones);
	Snapshot.RunSummary = RunSummary;

	Snapshot.Events = Events;
	Snapshot.Events.Sort([](const FLoop9ObservationEvent& A, const FLoop9ObservationEvent& B)
	{
		const int32 PriorityA = EventPriority(A.Type);
		const int32 PriorityB = EventPriority(B.Type);
		return PriorityA == PriorityB
			? A.Sequence > B.Sequence
			: PriorityA > PriorityB;
	});
	if (Snapshot.Events.Num() > MaxProjectedEvents)
	{
		Snapshot.Events.SetNum(MaxProjectedEvents);
	}
	return Snapshot;
}

int32 FLoop9ObservationJournalCore::EventPriority(ELoop9ObservationEventType Type)
{
	switch (Type)
	{
	case ELoop9ObservationEventType::CallCompleted: return 90;
	case ELoop9ObservationEventType::PursuerCaught: return 85;
	case ELoop9ObservationEventType::PursuerObserved: return 80;
	case ELoop9ObservationEventType::ObjectInspected: return 60;
	case ELoop9ObservationEventType::DoorDenied: return 55;
	case ELoop9ObservationEventType::DoorOpened:
	case ELoop9ObservationEventType::DoorClosed: return 45;
	case ELoop9ObservationEventType::ZoneEntered: return 40;
	case ELoop9ObservationEventType::FlashlightOn:
	case ELoop9ObservationEventType::FlashlightOff: return 20;
	default: return 0;
	}
}

uint16 FLoop9ObservationJournalCore::SaturatingSeconds(double Seconds)
{
	return static_cast<uint16>(FMath::Clamp(FMath::FloorToInt(FMath::Max(0.0, Seconds)), 0, MAX_uint16));
}

uint16 FLoop9ObservationJournalCore::SaturatingIncrement(uint16 Value)
{
	return Value == MAX_uint16 ? MAX_uint16 : Value + 1;
}

void FLoop9ObservationJournalCore::RememberVisitedZone(FName ZoneId)
{
	if (ZoneId.IsNone())
	{
		return;
	}
	VisitedZones.Remove(ZoneId);
	VisitedZones.Add(ZoneId);
	if (VisitedZones.Num() > MaxVisitedZones)
	{
		VisitedZones.RemoveAt(0, VisitedZones.Num() - MaxVisitedZones);
	}
}

void FLoop9ObservationJournalCore::EnforceEventCap()
{
	while (Events.Num() > MaxInternalEvents)
	{
		int32 EvictionIndex = 0;
		for (int32 Index = 1; Index < Events.Num(); ++Index)
		{
			const int32 CandidatePriority = EventPriority(Events[Index].Type);
			const int32 EvictionPriority = EventPriority(Events[EvictionIndex].Type);
			if (CandidatePriority < EvictionPriority
				|| (CandidatePriority == EvictionPriority
					&& Events[Index].Sequence < Events[EvictionIndex].Sequence))
			{
				EvictionIndex = Index;
			}
		}
		Events.RemoveAt(EvictionIndex);
	}
}
