#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "UObject/Object.h"
#include "Loop/LoopTypes.h"
#include "Runtime/Loop9ObservationJournal.h"
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
	/** Coarse place the AI may point at. Empty means it must not name a place. */
	FString AnomalyZone;
	/** Category of the affected object, never the actor name. */
	FString AnomalyObjectKind;
	/** Authored inactive place offered as a one-shot location misdirection target. */
	FString DecoyZone;
	bool bRepeatAnomaly = false;
	float Trust = 0.5f;
	float Kindness = 0.5f;
	float Suspicion = 0.2f;
	float Dependency = 0.2f;
	FDragojloCommitmentState AdviceState;
	TOptional<FLoop9ObservationSnapshot> ObservationSnapshot;
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
	EDragojloAdviceMode AdviceMode = EDragojloAdviceMode::None;
	EDragojloLiftAdvice LiftAdvice = EDragojloLiftAdvice::None;
	FString SuggestedZone;
	FString CommitmentId;
};

namespace Loop9ChatLimits
{
	/** Must stay aligned with backend ChatRequestMapper::MAX_MESSAGE_LENGTH. */
	// Mirrors backend ChatRequestMapper::MAX_MESSAGE_LENGTH (1000 since 08.09.2026).
	// Older cooks (≤ v1.0.5) still allow 4000 and get a clean 400 → static reply.
	inline constexpr int32 MaxMessageLength = 1000;
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
	static bool TryParseAdviceObject(const TSharedPtr<FJsonObject>& JsonResponse, FLoop9ChatResponse& OutResponse);
	static FString AdviceModeToWire(EDragojloAdviceMode Mode);
	static EDragojloAdviceMode AdviceModeFromWire(const FString& Wire);
	static FString LiftAdviceToWire(EDragojloLiftAdvice Advice);
	static EDragojloLiftAdvice LiftAdviceFromWire(const FString& Wire);
};
