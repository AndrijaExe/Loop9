#include "Runtime/DragojloMemory.h"

namespace
{
	constexpr int32 MaxCounter = 9999;

	int32 ClampCounter(int32 Value)
	{
		return FMath::Clamp(Value, 0, MaxCounter);
	}

	int32 ReadInt(const TMap<FString, FString>& Fields, const TCHAR* Key, int32 Fallback)
	{
		if (const FString* Value = Fields.Find(Key))
		{
			if (Value->IsNumeric())
			{
				return ClampCounter(FCString::Atoi(**Value));
			}
		}
		return Fallback;
	}
}

void FDragojloMemory::RecordRunFinished(
	ELoopEndingType EndingType,
	int32 TotalAIInteractions,
	float FinalKindness,
	const FDragojloCommitmentState& Commitment)
{
	RunsFinished = ClampCounter(RunsFinished + 1);
	LastEnding = EndingType;
	bHasLastEnding = true;
	LastRunCalls = ClampCounter(TotalAIInteractions);
	LastRunTone = ToneBucket(FinalKindness);

	const int32 LiesThisRun =
		(Commitment.bLocationMisdirectionUsed ? 1 : 0)
		+ (Commitment.bStaleFloorUsed ? 1 : 0)
		+ FMath::Max(0, Commitment.WrongLiftAdviceCount);
	LiesTold = ClampCounter(LiesTold + LiesThisRun);

	if (Commitment.bContradictionExposed)
	{
		CaughtLying = ClampCounter(CaughtLying + 1);
	}
	if (Commitment.FollowedLiftAdviceCount > 0)
	{
		RunsFollowingHim = ClampCounter(RunsFollowingHim + 1);
	}
}

FString FDragojloMemory::ToPersistedString() const
{
	if (IsEmpty())
	{
		return FString();
	}

	FString Out = FString::Printf(
		TEXT("runs=%d;calls=%d;tone=%d;lies=%d;caught=%d;followed=%d"),
		RunsFinished, LastRunCalls, LastRunTone, LiesTold, CaughtLying, RunsFollowingHim);
	if (bHasLastEnding)
	{
		Out += TEXT(";last=") + EndingWireLabel(LastEnding);
	}
	return Out;
}

FDragojloMemory FDragojloMemory::FromPersistedString(const FString& Raw)
{
	FDragojloMemory Memory;
	if (Raw.TrimStartAndEnd().IsEmpty())
	{
		return Memory;
	}

	TMap<FString, FString> Fields;
	TArray<FString> Pairs;
	Raw.ParseIntoArray(Pairs, TEXT(";"), true);
	for (const FString& Pair : Pairs)
	{
		FString Key;
		FString Value;
		if (Pair.Split(TEXT("="), &Key, &Value))
		{
			Fields.Add(Key.TrimStartAndEnd().ToLower(), Value.TrimStartAndEnd());
		}
	}

	Memory.RunsFinished = ReadInt(Fields, TEXT("runs"), 0);
	Memory.LastRunCalls = ReadInt(Fields, TEXT("calls"), 0);
	Memory.LiesTold = ReadInt(Fields, TEXT("lies"), 0);
	Memory.CaughtLying = ReadInt(Fields, TEXT("caught"), 0);
	Memory.RunsFollowingHim = ReadInt(Fields, TEXT("followed"), 0);

	if (const FString* Tone = Fields.Find(TEXT("tone")))
	{
		Memory.LastRunTone = FMath::Clamp(FCString::Atoi(**Tone), -1, 1);
	}

	if (const FString* Last = Fields.Find(TEXT("last")))
	{
		ELoopEndingType Parsed;
		if (TryParseEndingWireLabel(*Last, Parsed))
		{
			Memory.LastEnding = Parsed;
			Memory.bHasLastEnding = true;
		}
	}

	if (Memory.RunsFinished <= 0)
	{
		return FDragojloMemory();
	}
	return Memory;
}

FString FDragojloMemory::EndingWireLabel(ELoopEndingType EndingType)
{
	switch (EndingType)
	{
	case ELoopEndingType::EscapeTogether: return TEXT("escape_together");
	case ELoopEndingType::ObedientFool: return TEXT("obedient_fool");
	case ELoopEndingType::ColdBetrayal: return TEXT("cold_betrayal");
	case ELoopEndingType::ParanoidSurvivor: return TEXT("paranoid_survivor");
	case ELoopEndingType::MergedMemory: return TEXT("merged_memory");
	case ELoopEndingType::TheReplacement: return TEXT("the_replacement");
	case ELoopEndingType::TheExit: return TEXT("the_exit");
	default: return TEXT("paranoid_survivor");
	}
}

bool FDragojloMemory::TryParseEndingWireLabel(const FString& Label, ELoopEndingType& OutEnding)
{
	static const ELoopEndingType AllEndings[] = {
		ELoopEndingType::EscapeTogether,
		ELoopEndingType::ObedientFool,
		ELoopEndingType::ColdBetrayal,
		ELoopEndingType::ParanoidSurvivor,
		ELoopEndingType::MergedMemory,
		ELoopEndingType::TheReplacement,
		ELoopEndingType::TheExit,
	};

	const FString Wanted = Label.TrimStartAndEnd().ToLower();
	for (const ELoopEndingType Candidate : AllEndings)
	{
		if (Wanted == EndingWireLabel(Candidate))
		{
			OutEnding = Candidate;
			return true;
		}
	}
	return false;
}
