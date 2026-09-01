#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Loop/LoopEndingEvaluator.h"

namespace
{
	FEndingEvaluationContext Ctx(
		float Trust, float Kindness, float Cooperation,
		float Suspicion, float Dependency, float Stability,
		int32 AI, int32 Advances)
	{
		FEndingEvaluationContext Out;
		Out.Trust = Trust;
		Out.Kindness = Kindness;
		Out.Cooperation = Cooperation;
		Out.Suspicion = Suspicion;
		Out.Dependency = Dependency;
		Out.AIStability = Stability;
		Out.TotalAIInteractions = AI;
		Out.TotalAdvances = Advances;
		return Out;
	}

	int32 AsInt(ELoopEndingType Value)
	{
		return static_cast<int32>(Value);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoop9EndingEvaluatorProfilesTest,
	"Loop9.Runtime.Endings.EvaluatorProfiles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FLoop9EndingEvaluatorProfilesTest::RunTest(const FString&)
{
	struct FCase
	{
		const TCHAR* Name;
		FEndingEvaluationContext Context;
		ELoopEndingType Expected;
	};

	const FCase Cases[] = {
		// EndingSetup fixtures from LoopManagerSubsystem::ApplyEndingTestSetup
		{ TEXT("setup EscapeTogether"), Ctx(0.75f, 0.65f, 0.70f, 0.25f, 0.40f, 0.85f, 5, 9), ELoopEndingType::EscapeTogether },
		{ TEXT("setup ObedientFool"), Ctx(0.65f, 0.50f, 0.55f, 0.25f, 0.60f, 0.75f, 9, 9), ELoopEndingType::ObedientFool },
		{ TEXT("setup ColdBetrayal"), Ctx(0.70f, 0.30f, 0.65f, 0.35f, 0.45f, 0.75f, 6, 9), ELoopEndingType::ColdBetrayal },
		{ TEXT("setup ParanoidSurvivor"), Ctx(0.30f, 0.40f, 0.40f, 0.70f, 0.20f, 0.60f, 4, 9), ELoopEndingType::ParanoidSurvivor },
		{ TEXT("setup MergedMemory"), Ctx(0.55f, 0.65f, 0.70f, 0.30f, 0.45f, 0.65f, 7, 9), ELoopEndingType::MergedMemory },
		{ TEXT("setup TheReplacement"), Ctx(0.70f, 0.70f, 0.75f, 0.25f, 0.70f, 0.60f, 12, 9), ELoopEndingType::TheReplacement },

		// Hard gate: ignore Dragojlo
		{ TEXT("zero chats"), Ctx(0.50f, 0.50f, 0.50f, 0.20f, 0.20f, 1.00f, 0, 9), ELoopEndingType::ParanoidSurvivor },
		{ TEXT("two chats"), Ctx(0.54f, 0.50f, 0.52f, 0.22f, 0.16f, 0.90f, 2, 9), ELoopEndingType::ParanoidSurvivor },

		// Old waterfall near-misses that used to fall to Paranoid
		{ TEXT("first-run good"), Ctx(0.72f, 0.58f, 0.66f, 0.16f, 0.32f, 0.88f, 5, 9), ELoopEndingType::EscapeTogether },
		{ TEXT("first-run near miss"), Ctx(0.58f, 0.50f, 0.58f, 0.28f, 0.38f, 0.78f, 5, 9), ELoopEndingType::EscapeTogether },
		{ TEXT("trust below old 0.62 gate"), Ctx(0.58f, 0.50f, 0.58f, 0.22f, 0.36f, 0.80f, 4, 9), ELoopEndingType::EscapeTogether },
		{ TEXT("two rude but followed him"), Ctx(0.62f, 0.36f, 0.58f, 0.32f, 0.38f, 0.78f, 5, 9), ELoopEndingType::ColdBetrayal },
		{ TEXT("asked him often"), Ctx(0.60f, 0.48f, 0.52f, 0.28f, 0.46f, 0.78f, 7, 9), ELoopEndingType::ObedientFool },
		{ TEXT("kind but messy run"), Ctx(0.52f, 0.60f, 0.64f, 0.35f, 0.38f, 0.74f, 6, 9), ELoopEndingType::MergedMemory },
		{ TEXT("long clingy unstable"), Ctx(0.66f, 0.62f, 0.68f, 0.22f, 0.55f, 0.68f, 9, 9), ELoopEndingType::TheReplacement },
		{ TEXT("hostile after talking"), Ctx(0.35f, 0.38f, 0.42f, 0.60f, 0.22f, 0.70f, 4, 9), ELoopEndingType::ParanoidSurvivor },
	};

	bool bAllPassed = true;
	for (const FCase& Case : Cases)
	{
		const ELoopEndingType Got = FLoopEndingEvaluator::Evaluate(Case.Context);
		if (!TestEqual(Case.Name, AsInt(Got), AsInt(Case.Expected)))
		{
			bAllPassed = false;
		}
	}
	return bAllPassed;
}

#endif
