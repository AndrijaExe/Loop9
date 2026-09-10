#include "Runtime/Loop9CloudSaveFormat.h"

#include "Misc/ConfigCacheIni.h"

namespace
{
	constexpr const TCHAR* const PersistedKeyList[] =
	{
		Loop9CloudSaveFormat::SeenEndingsKey,
		Loop9CloudSaveFormat::SpottedAnomaliesKey,
		Loop9CloudSaveFormat::PendingUnlocksKey,
		Loop9CloudSaveFormat::DragojloMemoryKey,
	};
}

TArrayView<const TCHAR* const> Loop9CloudSaveFormat::PersistedKeys()
{
	return MakeArrayView(PersistedKeyList);
}

bool Loop9CloudSaveFormat::IsPersistedKey(const TCHAR* Key)
{
	for (const TCHAR* Candidate : PersistedKeys())
	{
		if (FCString::Strcmp(Candidate, Key) == 0)
		{
			return true;
		}
	}
	return false;
}

FString Loop9CloudSaveFormat::BuildIniText(const FConfigFile& File)
{
	FString Text = FString::Printf(TEXT("[%s]\r\n%s=1\r\n"), Section, CloudReadyKey);
	for (const TCHAR* Key : PersistedKeys())
	{
		FString Value;
		File.GetString(Section, Key, Value);
		Text += FString::Printf(TEXT("%s=%s\r\n"), Key, *Value);
	}
	return Text;
}
