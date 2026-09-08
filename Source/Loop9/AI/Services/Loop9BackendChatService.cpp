#include "AI/Services/Loop9BackendChatService.h"

#include "AI/Services/Loop9ObservationCodec.h"
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

bool ULoop9BackendChatService::TryExtractStateDeltas(const FString& RawContent, FString& OutReply, int32& OutKindnessDelta, int32& OutSuspicionDelta, int32& OutDependencyDelta)
{
	OutReply = RawContent;
	OutKindnessDelta = 0;
	OutSuspicionDelta = 0;
	OutDependencyDelta = 0;

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
	int32 ParsedD = 0;
	const bool bHasK = TryParseIntAfterToken(MetaPart, TEXT("KINDNESS="), ParsedK) || TryParseIntAfterToken(MetaPart, TEXT("KINDNESS:"), ParsedK);
	const bool bHasS = TryParseIntAfterToken(MetaPart, TEXT("SUSPICION="), ParsedS) || TryParseIntAfterToken(MetaPart, TEXT("SUSPICION:"), ParsedS);
	const bool bHasD = TryParseIntAfterToken(MetaPart, TEXT("DEPENDENCY="), ParsedD) || TryParseIntAfterToken(MetaPart, TEXT("DEPENDENCY:"), ParsedD);

	if (!bHasK && !bHasS && !bHasD)
	{
		return false;
	}

	OutReply = ReplyPart.IsEmpty() ? RawContent : ReplyPart;
	OutKindnessDelta = FMath::Clamp(ParsedK, -1, 1);
	OutSuspicionDelta = FMath::Clamp(ParsedS, -1, 1);
	OutDependencyDelta = bHasD ? FMath::Clamp(ParsedD, -1, 1) : 0;
	return true;
}

TSharedPtr<FJsonObject> ULoop9BackendChatService::BuildRunHistoryObject(const FDragojloMemory& Memory)
{
	if (Memory.IsEmpty())
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> History = MakeShareable(new FJsonObject());
	History->SetNumberField(TEXT("runs_finished"), Memory.RunsFinished);
	if (Memory.bHasLastEnding)
	{
		History->SetStringField(TEXT("last_ending"), FDragojloMemory::EndingWireLabel(Memory.LastEnding));
	}
	History->SetNumberField(TEXT("last_run_calls"), Memory.LastRunCalls);
	History->SetStringField(TEXT("last_run_tone"),
		Memory.LastRunTone > 0 ? TEXT("warm") : (Memory.LastRunTone < 0 ? TEXT("cold") : TEXT("neutral")));
	History->SetNumberField(TEXT("lies_told"), Memory.LiesTold);
	History->SetNumberField(TEXT("caught_lying"), Memory.CaughtLying);
	History->SetNumberField(TEXT("runs_following_him"), Memory.RunsFollowingHim);
	return History;
}

FString ULoop9BackendChatService::AdviceModeToWire(EDragojloAdviceMode Mode)
{
	switch (Mode)
	{
	case EDragojloAdviceMode::Withhold: return TEXT("withhold");
	case EDragojloAdviceMode::AccurateHint: return TEXT("accurate_hint");
	case EDragojloAdviceMode::MisdirectLocation: return TEXT("misdirect_location");
	case EDragojloAdviceMode::Confrontation: return TEXT("confrontation");
	case EDragojloAdviceMode::WrongLift: return TEXT("wrong_lift");
	case EDragojloAdviceMode::AccurateLift: return TEXT("accurate_lift");
	case EDragojloAdviceMode::StaleFloor: return TEXT("stale_floor");
	default: return TEXT("none");
	}
}

EDragojloAdviceMode ULoop9BackendChatService::AdviceModeFromWire(const FString& Wire)
{
	if (Wire.Equals(TEXT("withhold"), ESearchCase::IgnoreCase)) return EDragojloAdviceMode::Withhold;
	if (Wire.Equals(TEXT("accurate_hint"), ESearchCase::IgnoreCase)) return EDragojloAdviceMode::AccurateHint;
	if (Wire.Equals(TEXT("misdirect_location"), ESearchCase::IgnoreCase)) return EDragojloAdviceMode::MisdirectLocation;
	if (Wire.Equals(TEXT("confrontation"), ESearchCase::IgnoreCase)) return EDragojloAdviceMode::Confrontation;
	if (Wire.Equals(TEXT("wrong_lift"), ESearchCase::IgnoreCase)) return EDragojloAdviceMode::WrongLift;
	if (Wire.Equals(TEXT("accurate_lift"), ESearchCase::IgnoreCase)) return EDragojloAdviceMode::AccurateLift;
	if (Wire.Equals(TEXT("stale_floor"), ESearchCase::IgnoreCase)) return EDragojloAdviceMode::StaleFloor;
	return EDragojloAdviceMode::None;
}

FString ULoop9BackendChatService::LiftAdviceToWire(EDragojloLiftAdvice Advice)
{
	switch (Advice)
	{
	case EDragojloLiftAdvice::Lit: return TEXT("lit");
	case EDragojloLiftAdvice::Dark: return TEXT("dark");
	default: return TEXT("none");
	}
}

EDragojloLiftAdvice ULoop9BackendChatService::LiftAdviceFromWire(const FString& Wire)
{
	if (Wire.Equals(TEXT("lit"), ESearchCase::IgnoreCase)) return EDragojloLiftAdvice::Lit;
	if (Wire.Equals(TEXT("dark"), ESearchCase::IgnoreCase)) return EDragojloLiftAdvice::Dark;
	return EDragojloLiftAdvice::None;
}

bool ULoop9BackendChatService::TryParseAdviceObject(const TSharedPtr<FJsonObject>& JsonResponse, FLoop9ChatResponse& OutResponse)
{
	if (!JsonResponse.IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* AdviceObject = nullptr;
	if (!JsonResponse->TryGetObjectField(TEXT("advice"), AdviceObject) || !AdviceObject || !AdviceObject->IsValid())
	{
		return false;
	}

	FString ModeWire;
	if ((*AdviceObject)->TryGetStringField(TEXT("mode"), ModeWire))
	{
		OutResponse.AdviceMode = AdviceModeFromWire(ModeWire);
	}

	FString LiftWire;
	if ((*AdviceObject)->TryGetStringField(TEXT("lift"), LiftWire))
	{
		OutResponse.LiftAdvice = LiftAdviceFromWire(LiftWire);
	}

	(*AdviceObject)->TryGetStringField(TEXT("suggested_zone"), OutResponse.SuggestedZone);
	(*AdviceObject)->TryGetStringField(TEXT("commitment_id"), OutResponse.CommitmentId);
	return OutResponse.AdviceMode != EDragojloAdviceMode::None;
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

	// Omitted entirely when unauthored, which keeps the prompt exactly as it was
	// before any zone was filled in and lets the level be tagged one part at a time.
	if (!Context.AnomalyZone.IsEmpty() || !Context.AnomalyObjectKind.IsEmpty())
	{
		TSharedPtr<FJsonObject> DetailObject = MakeShareable(new FJsonObject());
		if (!Context.AnomalyZone.IsEmpty())
		{
			DetailObject->SetStringField(TEXT("zone"), Context.AnomalyZone);
		}
		if (!Context.AnomalyObjectKind.IsEmpty())
		{
			DetailObject->SetStringField(TEXT("object"), Context.AnomalyObjectKind);
		}
		JsonObject->SetObjectField(TEXT("anomaly_detail"), DetailObject);
	}

	if (!Context.DecoyZone.IsEmpty())
	{
		JsonObject->SetStringField(TEXT("decoy_zone"), Context.DecoyZone);
	}

	// Previous floor's anomaly, for the one-shot stale-floor slip. Omitted when
	// that floor was clean, which is exactly what stops him pointing at nothing.
	if (!Context.PreviousAnomalyZone.IsEmpty())
	{
		TSharedPtr<FJsonObject> PreviousObject = MakeShareable(new FJsonObject());
		PreviousObject->SetStringField(TEXT("zone"), Context.PreviousAnomalyZone);
		if (!Context.PreviousAnomalyObjectKind.IsEmpty())
		{
			PreviousObject->SetStringField(TEXT("object"), Context.PreviousAnomalyObjectKind);
		}
		JsonObject->SetObjectField(TEXT("previous_anomaly_detail"), PreviousObject);
	}

	TSharedPtr<FJsonObject> AdviceStateObject = MakeShareable(new FJsonObject());
	AdviceStateObject->SetBoolField(TEXT("location_misdirection_used"), Context.AdviceState.bLocationMisdirectionUsed);
	AdviceStateObject->SetBoolField(TEXT("contradiction_exposed"), Context.AdviceState.bContradictionExposed);
	AdviceStateObject->SetBoolField(TEXT("pending_decision_surrender"), Context.AdviceState.bPendingDecisionSurrender);
	AdviceStateObject->SetBoolField(TEXT("wrong_lift_used"), Context.AdviceState.bWrongLiftUsed);
	AdviceStateObject->SetBoolField(TEXT("followed_last_lift_advice"), Context.AdviceState.bFollowedLastLiftAdvice);
	AdviceStateObject->SetBoolField(TEXT("visited_suggested_decoy"), Context.AdviceState.bVisitedSuggestedDecoy);
	AdviceStateObject->SetBoolField(TEXT("confrontation_response_used"), Context.AdviceState.bConfrontationResponseUsed);
	AdviceStateObject->SetBoolField(TEXT("stale_floor_used"), Context.AdviceState.bStaleFloorUsed);
	AdviceStateObject->SetStringField(TEXT("last_advice_mode"), AdviceModeToWire(Context.AdviceState.LastAdviceMode));
	AdviceStateObject->SetStringField(TEXT("last_lift_advice"), LiftAdviceToWire(Context.AdviceState.LastLiftAdvice));
	if (!Context.AdviceState.LastSuggestedZone.IsEmpty())
	{
		AdviceStateObject->SetStringField(TEXT("last_suggested_zone"), Context.AdviceState.LastSuggestedZone);
	}
	JsonObject->SetObjectField(TEXT("advice_state"), AdviceStateObject);

	if (Context.ObservationSnapshot.IsSet())
	{
		const TSharedPtr<FJsonObject> SnapshotObject =
			FLoop9ObservationCodec::BuildSnapshotObject(
				Context.ObservationSnapshot.GetValue());
		if (SnapshotObject.IsValid())
		{
			JsonObject->SetObjectField(TEXT("observation_snapshot"), SnapshotObject);
		}
	}

	if (Context.RunHistory.IsSet() && !Context.RunHistory->IsEmpty())
	{
		const TSharedPtr<FJsonObject> HistoryObject = BuildRunHistoryObject(Context.RunHistory.GetValue());
		if (HistoryObject.IsValid())
		{
			JsonObject->SetObjectField(TEXT("run_history"), HistoryObject);
		}
	}

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
		const FString RetryAfterHeader = Response->GetHeader(TEXT("Retry-After"));
		if (!RetryAfterHeader.IsEmpty())
		{
			ChatResult.RetryAfterSeconds = FMath::Max(0, FCString::Atoi(*RetryAfterHeader));
		}

		if (ChatResult.HttpCode != 200)
		{
			ChatResult.ErrorMessage = FString::Printf(TEXT("Error: Backend returned HTTP %d"), ChatResult.HttpCode);

			TSharedPtr<FJsonObject> ErrorJson;
			TSharedRef<TJsonReader<>> ErrorReader = TJsonReaderFactory<>::Create(ResponseString);
			if (FJsonSerializer::Deserialize(ErrorReader, ErrorJson) && ErrorJson.IsValid())
			{
				const TSharedPtr<FJsonObject>* ErrorObject = nullptr;
				if (ErrorJson->TryGetObjectField(TEXT("error"), ErrorObject) && ErrorObject && ErrorObject->IsValid())
				{
					(*ErrorObject)->TryGetStringField(TEXT("code"), ChatResult.ErrorCode);
					FString ServerMessage;
					if ((*ErrorObject)->TryGetStringField(TEXT("message"), ServerMessage) && !ServerMessage.IsEmpty())
					{
						ChatResult.ErrorMessage = ServerMessage;
					}
				}
			}

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
		TryExtractStateDeltas(
			BackendMessage,
			CleanReply,
			ChatResult.KindnessDelta,
			ChatResult.SuspicionDelta,
			ChatResult.DependencyDelta);
		CleanReply = SanitizeReplyText(CleanReply);

		// A reply that was nothing but a [STATE] trailer sanitizes down to
		// nothing. Fail instead, so the loop message is refunded rather than
		// spent on an empty chat bubble.
		if (CleanReply.IsEmpty())
		{
			ChatResult.ErrorMessage = TEXT("Error: Invalid backend response format");
			Complete(ChatResult);
			return;
		}

		TryParseAdviceObject(JsonResponse, ChatResult);

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
