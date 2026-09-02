#include "Runtime/Loop9ObservationIds.h"

FName Loop9ObservationIds::Canonicalize(const FString& Identifier)
{
	FString Source = Identifier.TrimStartAndEnd().ToLower();
	if (Source.StartsWith(TEXT("the ")))
	{
		Source.RightChopInline(4);
	}

	FString Canonical;
	Canonical.Reserve(FMath::Min(Source.Len(), MaxLength));
	bool bPreviousSeparator = false;

	for (const TCHAR Character : Source)
	{
		if (Canonical.Len() >= MaxLength)
		{
			break;
		}

		const bool bAsciiAlphaNumeric =
			(Character >= TEXT('a') && Character <= TEXT('z'))
			|| (Character >= TEXT('0') && Character <= TEXT('9'));
		if (bAsciiAlphaNumeric)
		{
			Canonical.AppendChar(Character);
			bPreviousSeparator = false;
		}
		else if (!bPreviousSeparator && !Canonical.IsEmpty())
		{
			Canonical.AppendChar(TEXT('_'));
			bPreviousSeparator = true;
		}
	}

	while (Canonical.EndsWith(TEXT("_")))
	{
		Canonical.LeftChopInline(1, EAllowShrinking::No);
	}

	return Canonical.IsEmpty() ? NAME_None : FName(*Canonical);
}

FName Loop9ObservationIds::Canonicalize(FName Identifier)
{
	return Identifier.IsNone() ? NAME_None : Canonicalize(Identifier.ToString());
}
