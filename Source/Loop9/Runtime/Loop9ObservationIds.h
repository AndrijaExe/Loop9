#pragma once

#include "CoreMinimal.h"

namespace Loop9ObservationIds
{
	inline constexpr int32 MaxLength = 32;

	/**
	 * Converts authored observation labels and ids to one stable representation.
	 * A leading English "the " is intentionally ignored so backend labels match
	 * concise ids authored on observation volumes.
	 */
	LOOP9_API FName Canonicalize(const FString& Identifier);
	LOOP9_API FName Canonicalize(FName Identifier);
	inline FName Canonicalize(const TCHAR* Identifier)
	{
		return Canonicalize(FString(Identifier));
	}
}
