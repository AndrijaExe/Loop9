#include "Steam/Loop9SteamUtils.h"

#if LOOP9_WITH_STEAM
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END
#endif

bool FLoop9SteamUtils::IsRunningOnSteamDeck()
{
#if LOOP9_WITH_STEAM
	return SteamUtils() != nullptr && SteamUtils()->IsSteamRunningOnSteamDeck();
#else
	return false;
#endif
}

bool FLoop9SteamUtils::ShowOnScreenKeyboard(const FVector2D& FieldPositionPx, const FVector2D& FieldSizePx)
{
#if LOOP9_WITH_STEAM
	if (SteamUtils() == nullptr)
	{
		return false;
	}

	return SteamUtils()->ShowFloatingGamepadTextInput(
		k_EFloatingGamepadTextInputModeModeSingleLine,
		FMath::RoundToInt(FieldPositionPx.X),
		FMath::RoundToInt(FieldPositionPx.Y),
		FMath::Max(1, FMath::RoundToInt(FieldSizePx.X)),
		FMath::Max(1, FMath::RoundToInt(FieldSizePx.Y)));
#else
	return false;
#endif
}

void FLoop9SteamUtils::IndicateAchievementProgress(FName AchievementId, int32 Current, int32 Max)
{
#if LOOP9_WITH_STEAM
	if (SteamUserStats() == nullptr || AchievementId.IsNone() || Current <= 0 || Current >= Max)
	{
		return;
	}

	// Steam ignores this for unknown/already unlocked achievements, so it is
	// safe to call optimistically.
	SteamUserStats()->IndicateAchievementProgress(
		TCHAR_TO_UTF8(*AchievementId.ToString()),
		static_cast<uint32>(Current),
		static_cast<uint32>(Max));
#endif
}

bool FLoop9SteamUtils::UnlockAchievement(FName AchievementId)
{
#if LOOP9_WITH_STEAM
	if (SteamUserStats() == nullptr || AchievementId.IsNone())
	{
		return false;
	}

	const FString Name = AchievementId.ToString();

	// GetAchievement fails until Steam has this user's stats, so it doubles as
	// the readiness check that SetAchievement itself does not offer.
	bool bAlreadyUnlocked = false;
	if (!SteamUserStats()->GetAchievement(TCHAR_TO_UTF8(*Name), &bAlreadyUnlocked))
	{
		return false;
	}

	if (bAlreadyUnlocked)
	{
		return true;
	}

	if (!SteamUserStats()->SetAchievement(TCHAR_TO_UTF8(*Name)))
	{
		return false;
	}

	// Without StoreStats the unlock stays local and no toast is shown.
	return SteamUserStats()->StoreStats();
#else
	return false;
#endif
}

TArray<FName> FLoop9SteamUtils::UnlockedAchievements(const TArray<FName>& Candidates)
{
	TArray<FName> Unlocked;

#if LOOP9_WITH_STEAM
	if (SteamUserStats() == nullptr)
	{
		return Unlocked;
	}

	for (const FName& AchievementId : Candidates)
	{
		if (AchievementId.IsNone())
		{
			continue;
		}

		// GetAchievement returns false while Steam still lacks this user's
		// stats, which is why a miss is never read as "not unlocked".
		bool bAchieved = false;
		if (SteamUserStats()->GetAchievement(TCHAR_TO_UTF8(*AchievementId.ToString()), &bAchieved) && bAchieved)
		{
			Unlocked.Add(AchievementId);
		}
	}
#endif

	return Unlocked;
}
