#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Loop9RuntimePolicies.h"

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
	TestEqual(TEXT("Collapsed accusation wins tone"), Events[0].Tone, ERunEventTone::Suspicious);

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
		Loop9RuntimePolicies::ToneFromStateDeltas(-1, 0),
		ERunEventTone::Hostile);
	TestEqual(TEXT("Accusation is suspicious even if polite"),
		Loop9RuntimePolicies::ToneFromStateDeltas(1, 1),
		ERunEventTone::Suspicious);
	TestEqual(TEXT("Warmth is friendly"),
		Loop9RuntimePolicies::ToneFromStateDeltas(1, 0),
		ERunEventTone::Friendly);
	TestEqual(TEXT("Ordinary help request stays neutral"),
		Loop9RuntimePolicies::ToneFromStateDeltas(0, 0),
		ERunEventTone::Neutral);

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
	TestEqual(TEXT("Collapsed calls become hostile"), ToneEvents[0].Tone, ERunEventTone::Hostile);
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

#endif
