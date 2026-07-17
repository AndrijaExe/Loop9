#pragma once

#include "CoreMinimal.h"

/**
 * Shared helpers for deriving Loop9 backend API paths from a chat (or base) URL.
 * Keeps auth / chat / telemetry endpoints consistent across trailing-slash and host variants.
 */
namespace Loop9BackendEndpointUtils
{
	inline FString TrimTrailingSlashes(FString Url)
	{
		while (Url.EndsWith(TEXT("/")))
		{
			Url.LeftChopInline(1);
		}
		return Url;
	}

	/** Returns scheme://host[:port][/optional-base-path] without a trailing slash. */
	inline FString NormalizeBackendBaseUrl(const FString& ChatOrBaseUrl)
	{
		FString Base = TrimTrailingSlashes(ChatOrBaseUrl.TrimStartAndEnd());
		if (Base.IsEmpty())
		{
			return FString();
		}

		// API configuration may carry diagnostics/query metadata. Derived API
		// endpoints must operate on the URL path only.
		int32 QueryIndex = INDEX_NONE;
		int32 FragmentIndex = INDEX_NONE;
		Base.FindChar(TEXT('?'), QueryIndex);
		Base.FindChar(TEXT('#'), FragmentIndex);
		int32 MetadataIndex = INDEX_NONE;
		if (QueryIndex != INDEX_NONE && FragmentIndex != INDEX_NONE)
		{
			MetadataIndex = FMath::Min(QueryIndex, FragmentIndex);
		}
		else
		{
			MetadataIndex = QueryIndex != INDEX_NONE ? QueryIndex : FragmentIndex;
		}
		if (MetadataIndex != INDEX_NONE)
		{
			Base.LeftInline(MetadataIndex, EAllowShrinking::No);
			Base = TrimTrailingSlashes(Base);
		}

		// Strip known API suffixes so callers can pass either a base or a full chat URL.
		static const TCHAR* Suffixes[] = {
			TEXT("/api/chat"),
			TEXT("/api/auth/steam"),
			TEXT("/api/telemetry/run"),
			TEXT("/api"),
		};

		for (const TCHAR* Suffix : Suffixes)
		{
			if (Base.EndsWith(Suffix, ESearchCase::IgnoreCase))
			{
				Base.LeftChopInline(FCString::Strlen(Suffix));
				Base = TrimTrailingSlashes(Base);
				break;
			}
		}

		return Base;
	}

	inline FString BuildApiUrl(const FString& ChatOrBaseUrl, const TCHAR* AbsoluteApiPath)
	{
		const FString Base = NormalizeBackendBaseUrl(ChatOrBaseUrl);
		if (Base.IsEmpty() || AbsoluteApiPath == nullptr || AbsoluteApiPath[0] == TEXT('\0'))
		{
			return FString();
		}

		FString Path = AbsoluteApiPath;
		if (!Path.StartsWith(TEXT("/")))
		{
			Path = FString(TEXT("/")) + Path;
		}

		return Base + Path;
	}

	inline FString DeriveChatEndpoint(const FString& ChatOrBaseUrl)
	{
		return BuildApiUrl(ChatOrBaseUrl, TEXT("/api/chat"));
	}

	inline FString DeriveSteamAuthEndpoint(const FString& ChatOrBaseUrl)
	{
		return BuildApiUrl(ChatOrBaseUrl, TEXT("/api/auth/steam"));
	}

	inline FString DeriveTelemetryEndpoint(const FString& ChatOrBaseUrl)
	{
		return BuildApiUrl(ChatOrBaseUrl, TEXT("/api/telemetry/run"));
	}
}
