#include "UI/TypewriterHelper.h"

namespace
{
	TCHAR RandomWrongChar(TCHAR Correct)
	{
		const TCHAR Start = TEXT('a');
		const TCHAR End = TEXT('z');
		TCHAR Candidate = Correct;
		while (Candidate == Correct)
		{
			Candidate = static_cast<TCHAR>(FMath::RandRange(Start, End));
		}
		return Candidate;
	}
}

void FTypewriterHelper::Begin(FTypewriterState& State, const FString& InText, float InDuration, float InTypoProbability, const FString& InPrefix)
{
	State.Prefix = InPrefix;
	State.TargetText = InText;
	State.TypedText.Empty();
	State.CharIndex = 0;
	State.CharInterval = FMath::Max(0.02f, InDuration / FMath::Max(1, InText.Len()));
	State.TypoProbability = FMath::Clamp(InTypoProbability, 0.0f, 1.0f);
	State.bPendingCorrection = false;
	State.PendingCorrectChar = 0;
	State.bFinished = false;
}

float FTypewriterHelper::Step(FTypewriterState& State, FString& OutputText, bool& bPlayTypingSound)
{
	bPlayTypingSound = false;

	if (State.bFinished)
	{
		OutputText = State.Prefix + State.TypedText;
		return 0.0f;
	}

	if (State.bPendingCorrection)
	{
		if (!State.TypedText.IsEmpty())
		{
			State.TypedText.LeftChopInline(1);
		}
		State.TypedText.AppendChar(State.PendingCorrectChar);
		State.bPendingCorrection = false;
		State.CharIndex++;
		OutputText = State.Prefix + State.TypedText;
		bPlayTypingSound = true;
		return State.CharInterval;
	}

	if (State.CharIndex >= State.TargetText.Len())
	{
		State.bFinished = true;
		OutputText = State.Prefix + State.TypedText;
		return 0.0f;
	}

	const TCHAR CorrectChar = State.TargetText[State.CharIndex];
	const bool bCanTypo = FChar::IsAlpha(CorrectChar);
	const bool bShouldTypo = bCanTypo && (FMath::FRand() <= State.TypoProbability);

	if (bShouldTypo)
	{
		const TCHAR WrongChar = RandomWrongChar(FChar::ToLower(CorrectChar));
		State.TypedText.AppendChar(WrongChar);
		State.PendingCorrectChar = CorrectChar;
		State.bPendingCorrection = true;
		OutputText = State.Prefix + State.TypedText;
		bPlayTypingSound = false;
		return State.CharInterval * 0.45f;
	}

	State.TypedText.AppendChar(CorrectChar);
	State.CharIndex++;
	OutputText = State.Prefix + State.TypedText;
	bPlayTypingSound = true;
	return State.CharInterval;
}
