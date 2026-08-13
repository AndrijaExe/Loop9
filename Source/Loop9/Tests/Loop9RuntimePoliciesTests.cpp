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

#endif
