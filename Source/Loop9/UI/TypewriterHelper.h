#pragma once

#include "CoreMinimal.h"

struct FTypewriterState
{
	FString Prefix;
	FString TargetText;
	FString TypedText;
	int32 CharIndex = 0;
	float CharInterval = 0.04f;
	float TypoProbability = 0.0f;
	bool bPendingCorrection = false;
	TCHAR PendingCorrectChar = 0;
	bool bFinished = false;
};

class FTypewriterHelper
{
public:
	static void Begin(FTypewriterState& State, const FString& InText, float InDuration, float InTypoProbability, const FString& InPrefix = TEXT(""));

	// Returns delay until next step. OutputText is full text to render now.
	// bPlayTypingSound = true for normal/correction steps, false for typo-glitch frame.
	static float Step(FTypewriterState& State, FString& OutputText, bool& bPlayTypingSound);
};
