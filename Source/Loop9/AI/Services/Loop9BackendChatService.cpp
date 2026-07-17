#include "AI/Services/Loop9BackendChatService.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

namespace
{
	bool TryParseIntAfterToken(const FString& Source, const FString& Token, int32& OutValue)
	{
		int32 Index = Source.Find(Token, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		if (Index == INDEX_NONE)
		{
			return false;
		}

		FString Tail = Source.Mid(Index + Token.Len()).TrimStartAndEnd();
		if (Tail.IsEmpty())
		{
			return false;
		}

		FString Numeric;
		for (int32 i = 0; i < Tail.Len(); ++i)
		{
			const TCHAR C = Tail[i];
			if ((i == 0 && (C == TEXT('-') || C == TEXT('+'))) || FChar::IsDigit(C))
			{
				Numeric.AppendChar(C);
			}
			else
			{
				break;
			}
		}

		if (Numeric.IsEmpty())
		{
			return false;
		}

		OutValue = FCString::Atoi(*Numeric);
		return true;
	}
}

FString ULoop9BackendChatService::SanitizeReplyText(const FString& InText)
{
	FString Result = InText.TrimStartAndEnd();

	int32 CutIndex = INDEX_NONE;
	const int32 StateIdx = Result.Find(TEXT("[STATE]"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (StateIdx != INDEX_NONE)
	{
		CutIndex = StateIdx;
	}

	const int32 LegacyIdx = Result.Find(TEXT("&&"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (LegacyIdx != INDEX_NONE && (CutIndex == INDEX_NONE || LegacyIdx < CutIndex))
	{
		CutIndex = LegacyIdx;
	}

	const int32 TrustEchoIdx = Result.Find(TEXT("[Trust="), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (TrustEchoIdx != INDEX_NONE && (CutIndex == INDEX_NONE || TrustEchoIdx < CutIndex))
	{
		CutIndex = TrustEchoIdx;
	}

	if (CutIndex != INDEX_NONE)
	{
		Result = Result.Left(CutIndex).TrimStartAndEnd();
	}

	return Result;
}

bool ULoop9BackendChatService::TryExtractStateDeltas(const FString& RawContent, FString& OutReply, int32& OutKindnessDelta, int32& OutSuspicionDelta)
{
	OutReply = RawContent;
	OutKindnessDelta = 0;
	OutSuspicionDelta = 0;

	int32 MarkerIndex = RawContent.Find(TEXT("[STATE]"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (MarkerIndex == INDEX_NONE)
	{
		int32 DelimIndex = INDEX_NONE;
		if (!RawContent.FindLastChar(TEXT('&'), DelimIndex) || DelimIndex <= 0 || RawContent[DelimIndex - 1] != TEXT('&'))
		{
			return false;
		}

		const int32 LegacyMarkerIndex = DelimIndex - 1;
		FString ReplyPart = RawContent.Left(LegacyMarkerIndex).TrimStartAndEnd();
		FString MetaPart = RawContent.Mid(DelimIndex + 1).TrimStartAndEnd();

		int32 ParsedK = 0;
		if (MetaPart.StartsWith(TEXT("KINDNESS:"), ESearchCase::IgnoreCase))
		{
			ParsedK = FCString::Atoi(*MetaPart.RightChop(9).TrimStartAndEnd());
		}
		else if (MetaPart.Equals(TEXT("Kindness++"), ESearchCase::IgnoreCase))
		{
			ParsedK = 1;
		}
		else if (MetaPart.Equals(TEXT("Kindness--"), ESearchCase::IgnoreCase))
		{
			ParsedK = -1;
		}
		else
		{
			return false;
		}

		OutReply = ReplyPart.IsEmpty() ? RawContent : ReplyPart;
		OutKindnessDelta = FMath::Clamp(ParsedK, -1, 1);
		return true;
	}

	FString ReplyPart = RawContent.Left(MarkerIndex).TrimStartAndEnd();
	FString MetaPart = RawContent.Mid(MarkerIndex + 7).TrimStartAndEnd();

	int32 ParsedK = 0;
	int32 ParsedS = 0;
	const bool bHasK = TryParseIntAfterToken(MetaPart, TEXT("KINDNESS="), ParsedK) || TryParseIntAfterToken(MetaPart, TEXT("KINDNESS:"), ParsedK);
	const bool bHasS = TryParseIntAfterToken(MetaPart, TEXT("SUSPICION="), ParsedS) || TryParseIntAfterToken(MetaPart, TEXT("SUSPICION:"), ParsedS);

	if (!bHasK && !bHasS)
	{
		return false;
	}

	OutReply = ReplyPart.IsEmpty() ? RawContent : ReplyPart;
	OutKindnessDelta = FMath::Clamp(ParsedK, -1, 1);
	OutSuspicionDelta = FMath::Clamp(ParsedS, -1, 1);
	return true;
}

void ULoop9BackendChatService::SendChatRequest(const FLoop9ChatRequestContext& Context, FOnLoop9ChatResponseReceived OnComplete)
{
	FLoop9ChatResponse Result;

	if (Context.APIEndpoint.IsEmpty())
	{
		Result.ErrorMessage = TEXT("Error: API Endpoint not configured");
		OnComplete.ExecuteIfBound(Result);
		return;
	}

	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(Context.APIEndpoint);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!Context.RequestId.IsEmpty())
	{
		HttpRequest->SetHeader(TEXT("X-Request-Id"), Context.RequestId);
	}
	// End-to-end budget: backend AI deadline is 45s plus free-tier cold start and RTT headroom.
	HttpRequest->SetTimeout(65.0f);

	if (!Context.SessionToken.IsEmpty())
	{
		HttpRequest->SetHeader(TEXT("X-Session-Token"), Context.SessionToken);
	}
	else if (!Context.GameToken.IsEmpty())
	{
		HttpRequest->SetHeader(TEXT("X-Game-Token"), Context.GameToken);
	}

	// Session token already carries verified Steam player identity — don't send a redundant client id.
	if (Context.SessionToken.IsEmpty() && !Context.PlayerId.IsEmpty())
	{
		HttpRequest->AppendToHeader(TEXT("X-Player-Id"), Context.PlayerId);
	}

	const int32 KindnessDiscrete = (Context.Kindness > 0.66f) ? 1 : ((Context.Kindness < 0.34f) ? -1 : 0);
	const int32 SuspicionDiscrete = (Context.Suspicion > 0.66f) ? 1 : ((Context.Suspicion < 0.34f) ? -1 : 0);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField(TEXT("message"), Context.Message);
	JsonObject->SetStringField(TEXT("language"), Context.PreferredLanguage);
	JsonObject->SetNumberField(TEXT("ai_stability"), FMath::Clamp(Context.AIStability, 0.0f, 1.0f));
	JsonObject->SetNumberField(TEXT("loop_index"), FMath::Clamp(Context.LoopIndex, 1, 9));
	JsonObject->SetStringField(TEXT("anomaly_context"),
		Context.bRepeatAnomaly
			? FString::Printf(TEXT("%s Repeat anomaly from previous loop."), *Context.AnomalyContext)
			: Context.AnomalyContext);
	JsonObject->SetBoolField(TEXT("offtopic"), false);

	TSharedPtr<FJsonObject> StateObject = MakeShareable(new FJsonObject());
	StateObject->SetNumberField(TEXT("kindness"), KindnessDiscrete);
	StateObject->SetNumberField(TEXT("suspicion"), SuspicionDiscrete);
	StateObject->SetNumberField(TEXT("dependency"), FMath::Clamp(Context.Dependency, 0.0f, 1.0f));
	StateObject->SetNumberField(TEXT("player_confidence"), FMath::Clamp(Context.Trust, 0.0f, 1.0f));
	StateObject->SetBoolField(TEXT("repeat_anomaly"), Context.bRepeatAnomaly);
	StateObject->SetStringField(TEXT("anomaly_key"), Context.AnomalyKey);
	JsonObject->SetObjectField(TEXT("state"), StateObject);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(OutputString);

	const double StartedAt = FPlatformTime::Seconds();
	const FString RequestId = Context.RequestId;
	HttpRequest->OnProcessRequestComplete().BindLambda([OnComplete, RequestId, StartedAt](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		FLoop9ChatResponse ChatResult;
		auto Complete = [OnComplete, RequestId, StartedAt](const FLoop9ChatResponse& FinalResult)
		{
			const double DurationMs = (FPlatformTime::Seconds() - StartedAt) * 1000.0;
			UE_LOG(LogTemp, Log, TEXT("Chat request complete. RequestId=%s | Success=%s | HttpCode=%d | DurationMs=%.1f"),
				RequestId.IsEmpty() ? TEXT("(none)") : *RequestId,
				FinalResult.bSuccess ? TEXT("YES") : TEXT("NO"),
				FinalResult.HttpCode,
				DurationMs);
			OnComplete.ExecuteIfBound(FinalResult);
		};

		if (!bWasSuccessful || !Response.IsValid())
		{
			ChatResult.ErrorMessage = TEXT("Error: Request failed");
			Complete(ChatResult);
			return;
		}

		ChatResult.HttpCode = Response->GetResponseCode();
		const FString ResponseString = Response->GetContentAsString();

		if (ChatResult.HttpCode != 200)
		{
			ChatResult.ErrorMessage = FString::Printf(TEXT("Error: Backend returned HTTP %d"), ChatResult.HttpCode);
			Complete(ChatResult);
			return;
		}

		TSharedPtr<FJsonObject> JsonResponse;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid())
		{
			ChatResult.ErrorMessage = TEXT("Error: Invalid backend response format");
			Complete(ChatResult);
			return;
		}

		FString BackendMessage;
		if (!JsonResponse->TryGetStringField(TEXT("message"), BackendMessage) || BackendMessage.IsEmpty())
		{
			ChatResult.ErrorMessage = TEXT("Error: Invalid backend response format");
			Complete(ChatResult);
			return;
		}

		FString CleanReply = BackendMessage;
		TryExtractStateDeltas(BackendMessage, CleanReply, ChatResult.KindnessDelta, ChatResult.SuspicionDelta);
		CleanReply = SanitizeReplyText(CleanReply);

		ChatResult.bSuccess = true;
		ChatResult.Reply = CleanReply;
		Complete(ChatResult);
	});

	if (!HttpRequest->ProcessRequest())
	{
		FLoop9ChatResponse DispatchFailure;
		DispatchFailure.ErrorMessage = TEXT("Error: Request could not be started");
		UE_LOG(LogTemp, Warning, TEXT("Chat request could not start. RequestId=%s"),
			RequestId.IsEmpty() ? TEXT("(none)") : *RequestId);
		OnComplete.ExecuteIfBound(DispatchFailure);
	}
}
