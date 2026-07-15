#pragma once

#include "CoreMinimal.h"

/**
 * Thin wrappers around direct Steamworks calls that the Online Subsystem does
 * not expose. Every function is a safe no-op when Steam is not running
 * (non-Steam builds, editor without Steam, other platforms).
 */
struct FLoop9SteamUtils
{
	/** True when the game is running on a Steam Deck. */
	static bool IsRunningOnSteamDeck();

	/**
	 * Shows Steam's floating gamepad keyboard positioned so it avoids the given
	 * text field rect (absolute pixels). Only has an effect on Steam Deck /
	 * Big Picture; returns true if the keyboard was actually shown.
	 */
	static bool ShowOnScreenKeyboard(const FVector2D& FieldPositionPx, const FVector2D& FieldSizePx);

	/**
	 * Shows Steam's "x / y" progress toast for an achievement (e.g. 10/15
	 * messages towards ACH_HOTLINE). Purely visual - it does not store stats
	 * and does not unlock anything.
	 */
	static void IndicateAchievementProgress(FName AchievementId, int32 Current, int32 Max);
};
