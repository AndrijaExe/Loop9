#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Loop9RunEventCards.h"
#include "Runtime/Loop9RuntimePolicies.h"

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

#endif
