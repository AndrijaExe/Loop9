#pragma once

#include "CoreMinimal.h"
#include "Loop9ObservationJournal.generated.h"

UENUM()
enum class ELoop9ObservationEventType : uint8
{
	ZoneEntered,
	ObjectInspected,
	DoorOpened,
	DoorClosed,
	DoorDenied,
	FlashlightOn,
	FlashlightOff,
	PursuerObserved,
	PursuerCaught,
	CallCompleted
};

USTRUCT()
struct LOOP9_API FLoop9ObservationEvent
{
	GENERATED_BODY()

	ELoop9ObservationEventType Type = ELoop9ObservationEventType::ZoneEntered;
	FName ZoneId = NAME_None;
	FName SubjectId = NAME_None;
	uint8 Count = 1;
	uint16 AtSecond = 0;
	uint32 Sequence = 0;
};

USTRUCT()
struct LOOP9_API FLoop9ObservationRunSummary
{
	GENERATED_BODY()

	uint16 FloorsStarted = 0;
	uint16 AIInteractions = 0;
	uint16 ElevatorDecisions = 0;
	uint16 CorrectDecisions = 0;
};

USTRUCT()
struct LOOP9_API FLoop9ObservationSnapshot
{
	GENERATED_BODY()

	FName CurrentZone = NAME_None;
	uint16 SecondsOnFloor = 0;
	TArray<FLoop9ObservationEvent> Events;
	TArray<FName> VisitedZones;
	FLoop9ObservationRunSummary RunSummary;

	bool IsEmpty() const
	{
		return CurrentZone.IsNone()
			&& Events.IsEmpty()
			&& VisitedZones.IsEmpty()
			&& RunSummary.FloorsStarted == 0;
	}
};

/**
 * Pure, bounded observation memory. It stores only structured event types and
 * sanitized authored identifiers; it never stores chat, coordinates, actor
 * names, anomaly keys, commitment ids, or relationship values.
 */
class LOOP9_API FLoop9ObservationJournalCore
{
public:
	static constexpr int32 MaxInternalEvents = 16;
	static constexpr int32 MaxProjectedEvents = 8;
	static constexpr int32 MaxVisitedZones = 8;
	static constexpr int32 MaxIdentifierLength = 32;

	void ResetRun();
	void BeginFloor(int32 FloorIndex, double NowSeconds);
	void SetCurrentZone(FName ZoneId);
	void ClearCurrentZone(FName ZoneId);
	void Record(
		ELoop9ObservationEventType Type,
		FName ZoneId,
		FName SubjectId,
		double NowSeconds);
	void RecordAIInteraction(double NowSeconds);
	void RecordElevatorDecision(FName ChoiceId, bool bWasCorrect, double NowSeconds);

	FLoop9ObservationSnapshot BuildSnapshot(double NowSeconds) const;
	const TArray<FLoop9ObservationEvent>& GetInternalEvents() const { return Events; }
	FName GetCurrentZone() const { return CurrentZone; }
	int32 GetFloorIndex() const { return CurrentFloorIndex; }

	static FName SanitizeIdentifier(FName Identifier, FName Fallback = NAME_None);
	static FString EventTypeToWire(ELoop9ObservationEventType Type);
	static int32 EventPriority(ELoop9ObservationEventType Type);

private:
	static uint16 SaturatingSeconds(double Seconds);
	static uint16 SaturatingIncrement(uint16 Value);
	void RememberVisitedZone(FName ZoneId);
	void EnforceEventCap();

	TArray<FLoop9ObservationEvent> Events;
	TArray<FName> VisitedZones;
	FLoop9ObservationRunSummary RunSummary;
	FName CurrentZone = NAME_None;
	int32 CurrentFloorIndex = 0;
	double FloorStartedAt = 0.0;
	uint32 NextSequence = 0;
};
