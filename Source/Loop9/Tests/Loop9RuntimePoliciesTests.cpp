#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AI/Services/Loop9BackendChatService.h"
#include "AI/Services/Loop9ObservationCodec.h"
#include "Dom/JsonObject.h"
#include "Loop/LoopTypes.h"
#include "Runtime/DragojloCommitmentTracker.h"
#include "Runtime/Loop9ObservationIds.h"
#include "Runtime/Loop9ObservationJournal.h"
#include "Runtime/Loop9RunEventCards.h"
#include "Runtime/Loop9RuntimePolicies.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/** TestEqual has no scoped-enum overload; compare the underlying values. */
	template <typename EnumType>
	int32 AsInt(EnumType Value)
	{
		return static_cast<int32>(Value);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9SprintTimerPolicyTest,
	"Loop9.Runtime.Stamina.TimerLifecycle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9SprintTimerPolicyTest::RunTest(const FString&)
{
	TestFalse(
		TEXT("Full idle stamina does not need a timer"),
		Loop9RuntimePolicies::ShouldKeepSprintTimerActive(false, false, 3.0f, 3.0f));
	TestTrue(
		TEXT("Sprinting keeps the timer active"),
		Loop9RuntimePolicies::ShouldKeepSprintTimerActive(true, false, 3.0f, 3.0f));
	TestTrue(
		TEXT("Partially depleted stamina keeps recovery ticking"),
		Loop9RuntimePolicies::ShouldKeepSprintTimerActive(false, false, 1.5f, 3.0f));
	TestTrue(
		TEXT("Exhaustion recovery keeps the timer active"),
		Loop9RuntimePolicies::ShouldKeepSprintTimerActive(false, true, 3.0f, 3.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9PendingAchievementPolicyTest,
	"Loop9.Runtime.Achievements.PendingPersistence",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9PendingAchievementPolicyTest::RunTest(const FString&)
{
	const FName PendingId(TEXT("ACH_PENDING"));
	const FName InFlightId(TEXT("ACH_IN_FLIGHT"));
	TArray<FName> Pending;
	Pending.Add(PendingId);
	Pending.Add(PendingId);
	Pending.Add(NAME_None);
	TSet<FName> InFlight;
	InFlight.Add(PendingId);
	InFlight.Add(InFlightId);

	const TArray<FString> Persisted =
		Loop9RuntimePolicies::MergePendingAchievementIds(Pending, InFlight);

	TestEqual(TEXT("Pending and in-flight IDs are deduplicated"), Persisted.Num(), 2);
	TestTrue(TEXT("Pending ID is retained"), Persisted.Contains(PendingId.ToString()));
	TestTrue(TEXT("In-flight ID is retained"), Persisted.Contains(InFlightId.ToString()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9RunEventLogTest,
	"Loop9.Runtime.RunEvent.AppendAndClassify",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9RunEventLogTest::RunTest(const FString&)
{
	TArray<FRunEvent> Events;
	FRunEvent FirstCall;
	FirstCall.Type = ERunEventType::Call;
	FirstCall.LoopIndex = 4;
	FirstCall.KindnessDelta = 1;
	Loop9RuntimePolicies::AppendRunEvent(Events, FirstCall);

	FRunEvent SecondCall;
	SecondCall.Type = ERunEventType::Call;
	SecondCall.LoopIndex = 4;
	SecondCall.SuspicionDelta = 1;
	Loop9RuntimePolicies::AppendRunEvent(Events, SecondCall);

	TestEqual(TEXT("Calls on the same loop collapse"), Events.Num(), 1);
	TestEqual(TEXT("Collapsed call count is 2"), Events[0].Count, 2);
	TestEqual(TEXT("Collapsed call keeps warmth"), Events[0].KindnessDelta, 1);
	TestEqual(TEXT("Collapsed accusation wins tone"), AsInt(Events[0].Tone), AsInt(ERunEventTone::Suspicious));

	FRunEvent Lift;
	Lift.Type = ERunEventType::CorrectLift;
	Lift.LoopIndex = 4;
	Loop9RuntimePolicies::AppendRunEvent(Events, Lift);

	FRunEvent LaterCall;
	LaterCall.Type = ERunEventType::Call;
	LaterCall.LoopIndex = 5;
	Loop9RuntimePolicies::AppendRunEvent(Events, LaterCall);

	TestEqual(TEXT("Lift and later loop stay separate"), Events.Num(), 3);

	TestEqual(TEXT("Insult is hostile"),
		AsInt(Loop9RuntimePolicies::ToneFromStateDeltas(-1, 0)),
		AsInt(ERunEventTone::Hostile));
	TestEqual(TEXT("Accusation is suspicious even if polite"),
		AsInt(Loop9RuntimePolicies::ToneFromStateDeltas(1, 1)),
		AsInt(ERunEventTone::Suspicious));
	TestEqual(TEXT("Warmth is friendly"),
		AsInt(Loop9RuntimePolicies::ToneFromStateDeltas(1, 0)),
		AsInt(ERunEventTone::Friendly));
	TestEqual(TEXT("Ordinary help request stays neutral"),
		AsInt(Loop9RuntimePolicies::ToneFromStateDeltas(0, 0)),
		AsInt(ERunEventTone::Neutral));

	TArray<FRunEvent> ToneEvents;
	FRunEvent WarmCall;
	WarmCall.Type = ERunEventType::Call;
	WarmCall.LoopIndex = 2;
	WarmCall.KindnessDelta = 1;
	Loop9RuntimePolicies::AppendRunEvent(ToneEvents, WarmCall);

	FRunEvent CruelCall;
	CruelCall.Type = ERunEventType::Call;
	CruelCall.LoopIndex = 2;
	CruelCall.KindnessDelta = -1;
	Loop9RuntimePolicies::AppendRunEvent(ToneEvents, CruelCall);

	TestEqual(TEXT("Collapsed calls keep the harsher kindness"), ToneEvents[0].KindnessDelta, -1);
	TestEqual(TEXT("Collapsed calls become hostile"), AsInt(ToneEvents[0].Tone), AsInt(ERunEventTone::Hostile));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9ShiftArchivePolicyTest,
	"Loop9.Runtime.Archive.SeenEndings",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9ShiftArchivePolicyTest::RunTest(const FString&)
{
	TestEqual(TEXT("All six endings are listed"), Loop9RuntimePolicies::AllEndingTypes().Num(), 6);

	const TArray<FString> Seen = { TEXT("ACH_ENDING_ESCAPE_TOGETHER") };
	TestTrue(
		TEXT("Seen ending is unlocked"),
		Loop9RuntimePolicies::IsEndingUnlocked(Seen, TEXT("ACH_ENDING_ESCAPE_TOGETHER")));
	TestFalse(
		TEXT("Unseen ending stays locked"),
		Loop9RuntimePolicies::IsEndingUnlocked(Seen, TEXT("ACH_ENDING_THE_REPLACEMENT")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9MergePersistedValuesTest,
	"Loop9.Runtime.Archive.MergeValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9MergePersistedValuesTest::RunTest(const FString&)
{
	TArray<FString> Values = { TEXT("ACH_ENDING_ESCAPE_TOGETHER") };

	TestFalse(
		TEXT("Nothing new leaves the list alone"),
		Loop9RuntimePolicies::AddMissingValues(Values, { TEXT("ACH_ENDING_ESCAPE_TOGETHER") }));
	TestEqual(TEXT("A duplicate is not stored twice"), Values.Num(), 1);

	TestFalse(TEXT("Empty additions change nothing"), Loop9RuntimePolicies::AddMissingValues(Values, { TEXT("") }));

	TestTrue(
		TEXT("An ending only Steam knows is folded in"),
		Loop9RuntimePolicies::AddMissingValues(Values, { TEXT("ACH_ENDING_MERGED_MEMORY") }));
	TestEqual(TEXT("The recovered ending is kept"), Values.Num(), 2);
	TestEqual(TEXT("Existing entries keep their order"), Values[0], FString(TEXT("ACH_ENDING_ESCAPE_TOGETHER")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9AnomalyDetailSelectionTest,
	"Loop9.Runtime.Anomaly.DetailSelection",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9AnomalyDetailSelectionTest::RunTest(const FString&)
{
	using FCandidate = Loop9RuntimePolicies::FAnomalyDetailCandidate;

	TestFalse(TEXT("Nothing active yields nothing"), Loop9RuntimePolicies::SelectAnomalyDetail({}).IsSet());

	const FCandidate Unauthored = Loop9RuntimePolicies::SelectAnomalyDetail({ FCandidate{ 2, TEXT(""), TEXT("") } });
	TestFalse(TEXT("A component with no authored detail is skipped"), Unauthored.IsSet());

	// An unfilled higher-priority anomaly must not mask the one that is filled.
	const FCandidate PastTheBlank = Loop9RuntimePolicies::SelectAnomalyDetail({
		FCandidate{ 0, TEXT(""), TEXT("") },
		FCandidate{ 4, TEXT("the north corridor"), TEXT("a ceiling light panel") },
	});
	TestEqual(TEXT("Falls through to the authored candidate"), PastTheBlank.Zone, FString(TEXT("the north corridor")));

	// Several anomalies can run at once; the same world state must always send
	// the same prompt, so the lowest type priority wins regardless of order.
	const TArray<FCandidate> Competing = {
		FCandidate{ 5, TEXT("the stairwell landing"), TEXT("") },
		FCandidate{ 1, TEXT("the archive room"), TEXT("an office chair") },
		FCandidate{ 3, TEXT("the copier alcove"), TEXT("a wall clock") },
	};
	const FCandidate Winner = Loop9RuntimePolicies::SelectAnomalyDetail(Competing);
	TestEqual(TEXT("Lowest type priority wins"), Winner.Zone, FString(TEXT("the archive room")));
	TestEqual(TEXT("Winner carries its own object kind"), Winner.ObjectKind, FString(TEXT("an office chair")));

	// A phantom message has no place, only a kind, and must still be selectable.
	const FCandidate Placeless = Loop9RuntimePolicies::SelectAnomalyDetail({
		FCandidate{ 8, TEXT(""), TEXT("a message in this chat") },
	});
	TestTrue(TEXT("A placeless anomaly still counts"), Placeless.IsSet());
	TestTrue(TEXT("A placeless anomaly names no zone"), Placeless.Zone.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9DecoyZoneSelectionTest,
	"Loop9.Runtime.Anomaly.DecoyZoneSelection",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9DecoyZoneSelectionTest::RunTest(const FString&)
{
	TestTrue(
		TEXT("No inactive zones yields empty decoy"),
		Loop9RuntimePolicies::SelectDecoyZone({}, { TEXT("the archive room") }).IsEmpty());

	TestEqual(
		TEXT("Decoy skips zones that match an active place"),
		Loop9RuntimePolicies::SelectDecoyZone(
			{ TEXT("the archive room"), TEXT("the north corridor") },
			{ TEXT("the archive room") }),
		FString(TEXT("the north corridor")));

	TestEqual(
		TEXT("Decoy choice is deterministic by sorted unique zones"),
		Loop9RuntimePolicies::SelectDecoyZone(
			{ TEXT("the west wing"), TEXT("the north corridor"), TEXT("the north corridor") },
			{ TEXT("the archive room") }),
		FString(TEXT("the north corridor")));
	TestEqual(
		TEXT("Authored labels normalize to stable volume ids"),
		Loop9ObservationIds::Canonicalize(TEXT("The North Corridor")),
		FName(TEXT("north_corridor")));
	TestEqual(
		TEXT("Authored labels and concise ids are equivalent"),
		Loop9ObservationIds::Canonicalize(TEXT("The North Corridor")),
		Loop9ObservationIds::Canonicalize(FName(TEXT("north_corridor"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9CommitmentTrackerTest,
	"Loop9.Runtime.AI.CommitmentTracker",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9CommitmentTrackerTest::RunTest(const FString&)
{
	FDragojloCommitmentTracker Tracker;
	Tracker.ApplyAdvice(
		EDragojloAdviceMode::WrongLift,
		EDragojloLiftAdvice::Dark,
		FString(),
		TEXT("wrong-lift-1"),
		0,
		0,
		100.0);
	Tracker.ApplyAdvice(
		EDragojloAdviceMode::AccurateHint,
		EDragojloLiftAdvice::None,
		TEXT("The North Corridor"),
		TEXT("hint-2"),
		0,
		0,
		101.0);

	TestEqual(
		TEXT("Non-lift response preserves pending lift advice"),
		AsInt(Tracker.GetPendingLiftAdvice()),
		AsInt(EDragojloLiftAdvice::Dark));
	TestEqual(
		TEXT("Non-lift response does not reclassify pending advice"),
		AsInt(Tracker.GetPendingLiftAdviceMode()),
		AsInt(EDragojloAdviceMode::WrongLift));
	TestEqual(
		TEXT("Current response mode still updates"),
		AsInt(Tracker.GetState().LastAdviceMode),
		AsInt(EDragojloAdviceMode::AccurateHint));
	TestEqual(
		TEXT("Non-lift response preserves last actionable lift metadata"),
		AsInt(Tracker.GetState().LastLiftAdvice),
		AsInt(EDragojloLiftAdvice::Dark));

	Tracker.ApplyDecision(EButtonType::Increment);
	TestTrue(TEXT("Dark choice follows pending dark advice"),
		Tracker.GetState().bFollowedLastLiftAdvice);
	TestEqual(TEXT("Followed advice count increments once"),
		Tracker.GetState().FollowedLiftAdviceCount, 1);
	TestEqual(TEXT("Wrong-lift attribution survives later response"),
		Tracker.GetState().FollowedWrongLiftAdviceCount, 1);
	TestEqual(TEXT("Pending advice is consumed"),
		AsInt(Tracker.GetPendingLiftAdvice()),
		AsInt(EDragojloLiftAdvice::None));

	Tracker.ApplyDecision(EButtonType::Increment);
	TestEqual(TEXT("Consumed advice cannot count twice"),
		Tracker.GetState().FollowedLiftAdviceCount, 1);

	Tracker.ApplyAdvice(
		EDragojloAdviceMode::Withhold,
		EDragojloLiftAdvice::None,
		FString(),
		TEXT("surrender-1"),
		0,
		1,
		150.0);
	TestTrue(TEXT("Withheld surrender arms the current decision"),
		Tracker.GetState().bPendingDecisionSurrender);
	Tracker.ApplyDecision(EButtonType::Reset);
	TestFalse(TEXT("Any elevator decision clears an unused surrender"),
		Tracker.GetState().bPendingDecisionSurrender);

	Tracker.ApplyAdvice(
		EDragojloAdviceMode::MisdirectLocation,
		EDragojloLiftAdvice::None,
		TEXT("The North Corridor"),
		TEXT("decoy-1"),
		0,
		0,
		200.0);
	TestTrue(TEXT("Canonical concise zone records decoy visit"),
		Tracker.OnZoneEntered(FName(TEXT("north_corridor")), 204.5));
	TestEqual(TEXT("Decoy visit timing is retained"),
		Tracker.GetState().DecoyVisitSeconds, 4.5f);

	Tracker.Reset();
	TestEqual(TEXT("Reset clears commitment state"),
		Tracker.GetState().FollowedWrongLiftAdviceCount, 0);
	TestEqual(TEXT("Reset clears pending advice"),
		AsInt(Tracker.GetPendingLiftAdvice()),
		AsInt(EDragojloLiftAdvice::None));
	TestFalse(TEXT("Reset clears decoy target"),
		Tracker.OnZoneEntered(FName(TEXT("north_corridor")), 210.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9AdviceWireParsingTest,
	"Loop9.Runtime.AI.AdviceWireParsing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9AdviceWireParsingTest::RunTest(const FString&)
{
	TestEqual(
		TEXT("Misdirect wire round-trips"),
		ULoop9BackendChatService::AdviceModeToWire(EDragojloAdviceMode::MisdirectLocation),
		FString(TEXT("misdirect_location")));
	TestEqual(
		TEXT("Confrontation wire round-trips"),
		AsInt(ULoop9BackendChatService::AdviceModeFromWire(TEXT("confrontation"))),
		AsInt(EDragojloAdviceMode::Confrontation));
	TestEqual(
		TEXT("Wrong lift wire round-trips"),
		AsInt(ULoop9BackendChatService::AdviceModeFromWire(TEXT("wrong_lift"))),
		AsInt(EDragojloAdviceMode::WrongLift));
	TestEqual(
		TEXT("Dark lift wire round-trips"),
		AsInt(ULoop9BackendChatService::LiftAdviceFromWire(TEXT("dark"))),
		AsInt(EDragojloLiftAdvice::Dark));

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject());
	TSharedPtr<FJsonObject> Advice = MakeShareable(new FJsonObject());
	Advice->SetStringField(TEXT("mode"), TEXT("misdirect_location"));
	Advice->SetStringField(TEXT("lift"), TEXT("none"));
	Advice->SetStringField(TEXT("suggested_zone"), TEXT("the north corridor"));
	Advice->SetStringField(TEXT("commitment_id"), TEXT("abc123"));
	Root->SetObjectField(TEXT("advice"), Advice);

	FLoop9ChatResponse Parsed;
	TestTrue(TEXT("Optional advice object parses"), ULoop9BackendChatService::TryParseAdviceObject(Root, Parsed));
	TestEqual(TEXT("Parsed mode"), AsInt(Parsed.AdviceMode), AsInt(EDragojloAdviceMode::MisdirectLocation));
	TestEqual(TEXT("Parsed zone"), Parsed.SuggestedZone, FString(TEXT("the north corridor")));
	TestEqual(TEXT("Parsed commitment id"), Parsed.CommitmentId, FString(TEXT("abc123")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9RunEventCardCopyTest,
	"Loop9.Runtime.RunEvent.LocalizedCards",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9RunEventCardCopyTest::RunTest(const FString&)
{
	FRunEvent HostileAsk;
	HostileAsk.Type = ERunEventType::Call;
	HostileAsk.LoopIndex = 4;
	HostileAsk.KindnessDelta = -1;
	HostileAsk.DependencyDelta = 1;
	Loop9RuntimePolicies::FinalizeRunEvent(HostileAsk);

	const FRunEventCard HostileCard = Loop9RunEventCards::Build(HostileAsk);
	TestEqual(TEXT("Hostile card keeps call type"), AsInt(HostileCard.Type), AsInt(ERunEventType::Call));
	TestEqual(TEXT("Hostile card keeps tone"), AsInt(HostileCard.Tone), AsInt(ERunEventTone::Hostile));
	TestTrue(TEXT("Hostile title names the loop"), HostileCard.Title.ToString().Contains(TEXT("LOOP 4")));
	TestTrue(TEXT("Hostile title is a single call"), HostileCard.Title.ToString().Contains(TEXT("CALL")));
	TestFalse(TEXT("Single call is not labeled twice"), HostileCard.Title.ToString().Contains(TEXT("TWO")));
	TestTrue(TEXT("Hostile body mentions harshness"), HostileCard.Body.ToString().Contains(TEXT("harshly")));
	TestTrue(TEXT("Hostile ask mentions handing him the choice"), HostileCard.Body.ToString().Contains(TEXT("decide")));
	TestTrue(TEXT("Hostile ring is red"), HostileCard.RingColor.R > 0.7f && HostileCard.RingColor.G < 0.4f);

	TArray<FRunEvent> Collapsed;
	FRunEvent First;
	First.Type = ERunEventType::Call;
	First.LoopIndex = 4;
	First.KindnessDelta = 1;
	Loop9RuntimePolicies::AppendRunEvent(Collapsed, First);
	FRunEvent Second;
	Second.Type = ERunEventType::Call;
	Second.LoopIndex = 4;
	Second.SuspicionDelta = 1;
	Loop9RuntimePolicies::AppendRunEvent(Collapsed, Second);

	const FRunEventCard TwiceCard = Loop9RunEventCards::Build(Collapsed[0]);
	TestTrue(TEXT("Collapsed title says two calls"), TwiceCard.Title.ToString().Contains(TEXT("TWO CALLS")));
	TestTrue(TEXT("Collapsed body mentions calling twice"), TwiceCard.Body.ToString().Contains(TEXT("twice")));
	TestEqual(TEXT("Collapsed card count is 2"), TwiceCard.Count, 2);
	TestTrue(TEXT("Suspicious ring is yellow"), TwiceCard.RingColor.R > 0.8f && TwiceCard.RingColor.G > 0.7f);

	FRunEvent CorrectAnomaly;
	CorrectAnomaly.Type = ERunEventType::CorrectLift;
	CorrectAnomaly.LoopIndex = 3;
	CorrectAnomaly.bAnomalyExisted = true;
	const FRunEventCard CorrectCard = Loop9RunEventCards::Build(CorrectAnomaly);
	TestTrue(TEXT("Correct lift title"), CorrectCard.Title.ToString().Contains(TEXT("RIGHT ELEVATOR")));
	TestTrue(TEXT("Correct lift saw the anomaly"), CorrectCard.Body.ToString().Contains(TEXT("already wrong")));
	TestEqual(TEXT("Lift ring stays blue"), CorrectCard.RingColor, FLinearColor(0.50f, 0.80f, 1.00f));

	FRunEvent WrongClean;
	WrongClean.Type = ERunEventType::WrongLift;
	WrongClean.LoopIndex = 7;
	const FRunEventCard WrongCard = Loop9RunEventCards::Build(WrongClean);
	TestTrue(TEXT("Wrong lift title"), WrongCard.Title.ToString().Contains(TEXT("WRONG ELEVATOR")));
	TestTrue(TEXT("Wrong lift on a clean floor"), WrongCard.Body.ToString().Contains(TEXT("clean")));

	FRunEvent Ending;
	Ending.Type = ERunEventType::Ending;
	Ending.LoopIndex = 9;
	Ending.EndingType = ELoopEndingType::EscapeTogether;
	const FRunEventCard EndingCard = Loop9RunEventCards::Build(Ending);
	TestEqual(TEXT("Ending title reuses the ending name"), EndingCard.Title.ToString(), FString(TEXT("ESCAPE TOGETHER")));
	TestTrue(TEXT("Ending body stays short"), EndingCard.Body.ToString().Contains(TEXT("shift ended")));

	TArray<FRunEvent> Timeline;
	Loop9RuntimePolicies::AppendRunEvent(Timeline, HostileAsk);
	Loop9RuntimePolicies::AppendRunEvent(Timeline, CorrectAnomaly);
	Loop9RuntimePolicies::AppendRunEvent(Timeline, Ending);
	const TArray<FRunEventCard> Cards = Loop9RunEventCards::BuildAll(Timeline);
	TestEqual(TEXT("BuildAll keeps event order"), Cards.Num(), 3);
	TestEqual(TEXT("BuildAll first card is the call"), AsInt(Cards[0].Type), AsInt(ERunEventType::Call));
	TestEqual(TEXT("BuildAll last card is the ending"), AsInt(Cards[2].Type), AsInt(ERunEventType::Ending));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9ObservationJournalBoundsTest,
	"Loop9.Runtime.ObservationJournal.BoundsAndCoalescing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9ObservationJournalBoundsTest::RunTest(const FString&)
{
	FLoop9ObservationJournalCore Journal;
	Journal.BeginFloor(3, 100.0);

	for (int32 Index = 0; Index < 300; ++Index)
	{
		Journal.Record(
			ELoop9ObservationEventType::ObjectInspected,
			FName(TEXT("archive")),
			FName(TEXT("ledger")),
			101.0 + Index);
	}
	TestEqual(TEXT("Identical observations coalesce"), Journal.GetInternalEvents().Num(), 1);
	TestEqual(TEXT("Coalesced count saturates"), Journal.GetInternalEvents()[0].Count, MAX_uint8);
	TestEqual(TEXT("Coalesced event keeps latest second"), Journal.GetInternalEvents()[0].AtSecond, 300);

	Journal.BeginFloor(4, 500.0);
	for (int32 Index = 0; Index < 16; ++Index)
	{
		Journal.Record(
			ELoop9ObservationEventType::FlashlightOn,
			NAME_None,
			FName(*FString::Printf(TEXT("switch_%02d"), Index)),
			501.0 + Index);
	}
	Journal.RecordAIInteraction(550.0);
	TestEqual(TEXT("Internal journal stays hard capped"), Journal.GetInternalEvents().Num(), 16);
	TestTrue(TEXT("Higher-priority phone response survives eviction"),
		Journal.GetInternalEvents().ContainsByPredicate([](const FLoop9ObservationEvent& Event)
		{
			return Event.Type == ELoop9ObservationEventType::CallCompleted;
		}));
	TestFalse(TEXT("Oldest low-priority event is evicted"),
		Journal.GetInternalEvents().ContainsByPredicate([](const FLoop9ObservationEvent& Event)
		{
			return Event.SubjectId == FName(TEXT("switch_00"));
		}));
	const FLoop9ObservationSnapshot Projection = Journal.BuildSnapshot(560.0);
	TestEqual(TEXT("Outgoing projection stays capped"),
		Projection.Events.Num(),
		FLoop9ObservationJournalCore::MaxProjectedEvents);
	TestEqual(TEXT("Projection orders higher priority first"),
		AsInt(Projection.Events[0].Type),
		AsInt(ELoop9ObservationEventType::CallCompleted));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9ObservationJournalResetTest,
	"Loop9.Runtime.ObservationJournal.ResetsAndVisitedZones",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9ObservationJournalResetTest::RunTest(const FString&)
{
	FLoop9ObservationJournalCore Journal;
	Journal.BeginFloor(1, 10.0);
	Journal.Record(
		ELoop9ObservationEventType::ZoneEntered,
		FName(TEXT("The North Corridor")),
		NAME_None,
		10.5);
	TestEqual(TEXT("Journal stores the canonical current zone"),
		Journal.GetCurrentZone(), FName(TEXT("north_corridor")));
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Journal.Record(
			ELoop9ObservationEventType::ZoneEntered,
			FName(*FString::Printf(TEXT("zone_%02d"), Index)),
			NAME_None,
			11.0 + Index);
	}
	FLoop9ObservationSnapshot Snapshot = Journal.BuildSnapshot(25.9);
	TestEqual(TEXT("Visited zones stay capped"), Snapshot.VisitedZones.Num(), 8);
	TestEqual(TEXT("Most recently entered zone is current"), Snapshot.CurrentZone, FName(TEXT("zone_09")));
	TestEqual(TEXT("Floor seconds are integral and bounded"), Snapshot.SecondsOnFloor, 15);

	Journal.BeginFloor(2, 30.0);
	Snapshot = Journal.BuildSnapshot(31.0);
	TestTrue(TEXT("Floor reset clears events"), Snapshot.Events.IsEmpty());
	TestTrue(TEXT("Floor reset clears visited zones"), Snapshot.VisitedZones.IsEmpty());
	TestTrue(TEXT("Floor reset clears current zone"), Snapshot.CurrentZone.IsNone());
	TestEqual(TEXT("Floor reset preserves fixed run summary"), Snapshot.RunSummary.FloorsStarted, 2);
	Journal.RestoreCurrentZone(FName(TEXT("The North Corridor")));
	Snapshot = Journal.BuildSnapshot(31.5);
	TestEqual(TEXT("Physical occupancy can be restored after floor reset"),
		Snapshot.CurrentZone, FName(TEXT("north_corridor")));
	TestTrue(TEXT("Restored occupancy does not synthesize a visited zone"),
		Snapshot.VisitedZones.IsEmpty());
	TestTrue(TEXT("Restored occupancy does not synthesize an event"),
		Snapshot.Events.IsEmpty());

	Journal.RecordAIInteraction(32.0);
	Journal.RecordElevatorDecision(FName(TEXT("dark")), true, 33.0);
	Journal.ResetRun();
	Snapshot = Journal.BuildSnapshot(40.0);
	TestTrue(TEXT("Run reset clears the full snapshot"), Snapshot.IsEmpty());
	TestEqual(TEXT("Run reset clears AI count"), Snapshot.RunSummary.AIInteractions, 0);
	TestEqual(TEXT("Run reset clears decision count"), Snapshot.RunSummary.ElevatorDecisions, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9ObservationJournalProjectionTest,
	"Loop9.Runtime.ObservationJournal.DeterministicSanitizedBudget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9ObservationJournalProjectionTest::RunTest(const FString&)
{
	TestEqual(TEXT("Authored ids normalize safely"),
		Loop9ObservationIds::Canonicalize(TEXT(" North Hall / Actor.Secret:42 ")),
		FName(TEXT("north_hall_actor_secret_42")));
	TestTrue(TEXT("Empty unsafe ids are omitted"),
		Loop9ObservationIds::Canonicalize(TEXT("!!!")).IsNone());
	TestEqual(TEXT("Leading article is ignored consistently"),
		Loop9ObservationIds::Canonicalize(TEXT("The North Hall")),
		Loop9ObservationIds::Canonicalize(TEXT("north_hall")));
	TestEqual(TEXT("Denied doors use the backend wire contract"),
		FLoop9ObservationCodec::EventTypeToWire(
			ELoop9ObservationEventType::DoorDenied),
		FString(TEXT("door_denied")));
	TestEqual(TEXT("A verified pursuer catch uses the backend wire contract"),
		FLoop9ObservationCodec::EventTypeToWire(
			ELoop9ObservationEventType::PursuerCaught),
		FString(TEXT("pursuer_caught")));

	FLoop9ObservationJournalCore Journal;
	Journal.BeginFloor(5, 1000.0);
	for (int32 Index = 0; Index < 16; ++Index)
	{
		Journal.Record(
			Index % 2 == 0
				? ELoop9ObservationEventType::ObjectInspected
				: ELoop9ObservationEventType::DoorOpened,
			FName(*FString::Printf(TEXT("long_authored_zone_%02d"), Index)),
			FName(*FString::Printf(TEXT("long_authored_subject_%02d"), Index)),
			1001.0 + Index);
	}
	Journal.RecordAIInteraction(1018.0);
	const FLoop9ObservationSnapshot Snapshot = Journal.BuildSnapshot(1030.0);
	const FString First = FLoop9ObservationCodec::SerializeSnapshot(Snapshot);
	const FString Second = FLoop9ObservationCodec::SerializeSnapshot(Snapshot);
	TestEqual(TEXT("Projection serialization is deterministic"), First, Second);
	const FTCHARToUTF8 Utf8(*First);
	TestTrue(TEXT("Snapshot respects 1024-byte UTF-8 budget"), Utf8.Length() <= 1024);
	TestFalse(TEXT("Snapshot contains no actor-name punctuation"), First.Contains(TEXT("/")));
	TestFalse(TEXT("Snapshot contains no relationship floats"), First.Contains(TEXT("dependency")));
	TestTrue(TEXT("Snapshot keeps fixed run summary"), First.Contains(TEXT("\"run_summary\"")));
	TestTrue(TEXT("Snapshot uses the backend call event contract"),
		First.Contains(TEXT("\"type\":\"call_completed\"")));
	TestTrue(TEXT("Snapshot sends event age rather than internal floor timestamp"),
		First.Contains(TEXT("\"age_seconds\":")));
	TestFalse(TEXT("Snapshot never exposes the internal floor timestamp"),
		First.Contains(TEXT("\"at_second\":")));

	TSharedPtr<FJsonObject> Parsed;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(First);
	TestTrue(TEXT("Budgeted snapshot remains valid JSON"),
		FJsonSerializer::Deserialize(Reader, Parsed) && Parsed.IsValid());
	const TArray<TSharedPtr<FJsonValue>>* Events = nullptr;
	TestTrue(TEXT("Budgeted snapshot keeps an events array"),
		Parsed.IsValid() && Parsed->TryGetArrayField(TEXT("events"), Events));
	TestTrue(TEXT("Budget trimming never exceeds projection cap"),
		Events && Events->Num() <= FLoop9ObservationJournalCore::MaxProjectedEvents);
	const TSharedPtr<FJsonObject> DirectObject =
		FLoop9ObservationCodec::BuildSnapshotObject(Snapshot);
	TestTrue(TEXT("Codec exposes a direct JSON object"), DirectObject.IsValid());
	return true;
}

#endif
