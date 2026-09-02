#include "AI/Services/Loop9ObservationCodec.h"

#include "Runtime/Loop9ObservationIds.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString FLoop9ObservationCodec::EventTypeToWire(ELoop9ObservationEventType Type)
{
	switch (Type)
	{
	case ELoop9ObservationEventType::ZoneEntered: return TEXT("zone_entered");
	case ELoop9ObservationEventType::ObjectInspected: return TEXT("object_inspected");
	case ELoop9ObservationEventType::DoorOpened: return TEXT("door_opened");
	case ELoop9ObservationEventType::DoorClosed: return TEXT("door_closed");
	case ELoop9ObservationEventType::DoorDenied: return TEXT("door_denied");
	case ELoop9ObservationEventType::FlashlightOn: return TEXT("flashlight_on");
	case ELoop9ObservationEventType::FlashlightOff: return TEXT("flashlight_off");
	case ELoop9ObservationEventType::PursuerObserved: return TEXT("pursuer_observed");
	case ELoop9ObservationEventType::PursuerCaught: return TEXT("pursuer_caught");
	case ELoop9ObservationEventType::CallCompleted: return TEXT("call_completed");
	default: return TEXT("unknown");
	}
}

TSharedPtr<FJsonObject> FLoop9ObservationCodec::BuildSnapshotObject(
	const FLoop9ObservationSnapshot& Snapshot,
	int32 MaxUtf8Bytes)
{
	const int32 SafeBudget = FMath::Max(1, MaxUtf8Bytes);
	int32 EventCount = FMath::Min(
		Snapshot.Events.Num(),
		FLoop9ObservationJournalCore::MaxProjectedEvents);

	for (; EventCount >= 0; --EventCount)
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetStringField(
			TEXT("current_zone"),
			Loop9ObservationIds::Canonicalize(Snapshot.CurrentZone).ToString());
		Root->SetNumberField(TEXT("seconds_on_floor"), Snapshot.SecondsOnFloor);

		TArray<TSharedPtr<FJsonValue>> EventValues;
		for (int32 Index = 0; Index < EventCount; ++Index)
		{
			const FLoop9ObservationEvent& Event = Snapshot.Events[Index];
			TSharedRef<FJsonObject> EventObject = MakeShared<FJsonObject>();
			EventObject->SetStringField(TEXT("type"), EventTypeToWire(Event.Type));
			const FName SafeZone = Loop9ObservationIds::Canonicalize(Event.ZoneId);
			const FName SafeSubject = Loop9ObservationIds::Canonicalize(Event.SubjectId);
			if (!SafeZone.IsNone())
			{
				EventObject->SetStringField(TEXT("zone"), SafeZone.ToString());
			}
			if (!SafeSubject.IsNone())
			{
				EventObject->SetStringField(TEXT("subject"), SafeSubject.ToString());
			}
			EventObject->SetNumberField(TEXT("count"), Event.Count);
			EventObject->SetNumberField(
				TEXT("age_seconds"),
				FMath::Max(0, static_cast<int32>(Snapshot.SecondsOnFloor)
					- static_cast<int32>(Event.AtSecond)));
			EventValues.Add(MakeShared<FJsonValueObject>(EventObject));
		}
		Root->SetArrayField(TEXT("events"), EventValues);

		TArray<TSharedPtr<FJsonValue>> VisitedZoneValues;
		const int32 VisitedZoneCount = FMath::Min(
			Snapshot.VisitedZones.Num(),
			FLoop9ObservationJournalCore::MaxVisitedZones);
		for (int32 Index = 0; Index < VisitedZoneCount; ++Index)
		{
			const FName SafeZone =
				Loop9ObservationIds::Canonicalize(Snapshot.VisitedZones[Index]);
			if (!SafeZone.IsNone())
			{
				VisitedZoneValues.Add(MakeShared<FJsonValueString>(SafeZone.ToString()));
			}
		}
		Root->SetArrayField(TEXT("visited_zones"), VisitedZoneValues);

		TSharedRef<FJsonObject> SummaryObject = MakeShared<FJsonObject>();
		SummaryObject->SetNumberField(TEXT("floors_started"), Snapshot.RunSummary.FloorsStarted);
		SummaryObject->SetNumberField(TEXT("ai_interactions"), Snapshot.RunSummary.AIInteractions);
		SummaryObject->SetNumberField(TEXT("elevator_decisions"), Snapshot.RunSummary.ElevatorDecisions);
		SummaryObject->SetNumberField(TEXT("correct_decisions"), Snapshot.RunSummary.CorrectDecisions);
		Root->SetObjectField(TEXT("run_summary"), SummaryObject);

		const FString Output = SerializeObject(Root);
		const FTCHARToUTF8 Utf8(*Output);
		if (Utf8.Length() <= SafeBudget)
		{
			return Root;
		}
	}

	return nullptr;
}

FString FLoop9ObservationCodec::SerializeSnapshot(
	const FLoop9ObservationSnapshot& Snapshot,
	int32 MaxUtf8Bytes)
{
	const TSharedPtr<FJsonObject> Object = BuildSnapshotObject(Snapshot, MaxUtf8Bytes);
	return Object.IsValid() ? SerializeObject(Object.ToSharedRef()) : FString();
}

FString FLoop9ObservationCodec::SerializeObject(const TSharedRef<FJsonObject>& Object)
{
	FString Output;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Output);
	FJsonSerializer::Serialize(Object, Writer);
	return Output;
}
