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

	/**
	 * Unlocks an achievement straight through Steamworks and stores it, which is
	 * what makes the toast appear. Returns false when Steam is absent or has not
	 * handed us this user's stats yet, so the caller can retry or fall back to
	 * the online subsystem. Already-unlocked achievements report success.
	 */
	static bool UnlockAchievement(FName AchievementId);

	/**
	 * Reports which of the given achievements Steam already holds for this user.
	 * An empty result also means "Steam could not tell us", so callers merge this
	 * into their own record rather than treating it as the whole truth.
	 */
	static TArray<FName> UnlockedAchievements(const TArray<FName>& Candidates);

	/**
	 * Writes a small blob into Steam Remote Storage so Properties → General
	 * shows Cloud usage even if Auto-Cloud misses the AppData file.
	 */
	static bool WriteCloudFile(const FString& Filename, const FString& Contents);
};
