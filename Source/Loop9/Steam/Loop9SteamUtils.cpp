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
