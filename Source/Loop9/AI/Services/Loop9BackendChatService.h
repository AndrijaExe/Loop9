#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Loop9BackendChatService.generated.h"

struct FLoop9ChatRequestContext
{
	FString RequestId;
	FString Message;
	FString APIEndpoint;
	FString GameToken;
	/** Short-lived backend session token (Steam auth). Takes precedence over GameToken. */
	FString SessionToken;
	FString PlayerId;
	FString PreferredLanguage = TEXT("sr");
	float AIStability = 1.0f;
	int32 LoopIndex = 1;
	FString AnomalyContext;
	FString AnomalyKey;
	bool bRepeatAnomaly = false;
	float Trust = 0.5f;
	float Kindness = 0.5f;
	float Suspicion = 0.2f;
	float Dependency = 0.2f;
};

struct FLoop9ChatResponse
{
	bool bSuccess = false;
	FString Reply;
	int32 KindnessDelta = 0;
	int32 SuspicionDelta = 0;
	int32 DependencyDelta = 0;
	int32 HttpCode = 0;
	/** Seconds from Retry-After when rate limited; 0 when absent. */
	int32 RetryAfterSeconds = 0;
	FString ErrorMessage;
	FString ErrorCode;
};

namespace Loop9ChatLimits
{
	/** Must stay aligned with backend ChatRequestMapper::MAX_MESSAGE_LENGTH. */
	inline constexpr int32 MaxMessageLength = 4000;
}

DECLARE_DELEGATE_OneParam(FOnLoop9ChatResponseReceived, const FLoop9ChatResponse&);

UCLASS()
class LOOP9_API ULoop9BackendChatService : public UObject
{
	GENERATED_BODY()

public:
	void SendChatRequest(const FLoop9ChatRequestContext& Context, FOnLoop9ChatResponseReceived OnComplete);

	static FString SanitizeReplyText(const FString& InText);
	static bool TryExtractStateDeltas(const FString& RawContent, FString& OutReply, int32& OutKindnessDelta, int32& OutSuspicionDelta, int32& OutDependencyDelta);
};
