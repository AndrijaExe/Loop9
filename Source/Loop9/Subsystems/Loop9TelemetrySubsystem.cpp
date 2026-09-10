#include "Subsystems/Loop9TelemetrySubsystem.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/Loop9BackendAuthSubsystem.h"
#include "AI/Services/Loop9BackendEndpointUtils.h"

void ULoop9TelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Prefer config early so ending telemetry works even if AI_Friend hasn't begun play yet.
	FString ChatEndpoint;
	FString Token;
	if (GConfig)
	{
		GConfig->GetString(TEXT("/Script/Loop9.AI_Friend"), TEXT("APIEndpoint"), ChatEndpoint, GGameIni);
		GConfig->GetString(TEXT("/Script/Loop9.AI_Friend"), TEXT("GameToken"), Token, GGameIni);
	}

	if (!ChatEndpoint.IsEmpty())
	{
		ConfigureFromChatEndpoint(ChatEndpoint, Token);
	}
}

void ULoop9TelemetrySubsystem::Deinitialize()
{
	ClearPendingTelemetryAuth();
	Super::Deinitialize();
}

void ULoop9TelemetrySubsystem::ConfigureFromChatEndpoint(const FString& ChatEndpoint, const FString& InGameToken)
{
	TelemetryEndpoint = Loop9BackendEndpointUtils::DeriveTelemetryEndpoint(ChatEndpoint);
	if (!InGameToken.IsEmpty())
	{
		GameToken = InGameToken;
	}

	UE_LOG(LogTemp, Log, TEXT("Telemetry configured. Endpoint=%s | TokenSet=%s"),
		TelemetryEndpoint.IsEmpty() ? TEXT("(empty)") : *TelemetryEndpoint,
		GameToken.IsEmpty() ? TEXT("NO") : TEXT("YES"));
}

FString ULoop9TelemetrySubsystem::EndingTelemetryId(ELoopEndingType EndingType)
{
	switch (EndingType)
	{
	case ELoopEndingType::EscapeTogether: return TEXT("escape_together");
	case ELoopEndingType::ObedientFool: return TEXT("obedient_fool");
	case ELoopEndingType::ColdBetrayal: return TEXT("cold_betrayal");
	case ELoopEndingType::MergedMemory: return TEXT("merged_memory");
	case ELoopEndingType::TheReplacement: return TEXT("the_replacement");
	case ELoopEndingType::ParanoidSurvivor: return TEXT("paranoid_survivor");
	case ELoopEndingType::TheExit: return TEXT("the_exit");
	default: return FString();
	}
}

void ULoop9TelemetrySubsystem::SendRunFinished(
	ELoopEndingType EndingType,
	int32 TotalResets,
	int32 TotalAIInteractions,
	const FDragojloCommitmentState& Commitment)
{
	const FString EndingId = EndingTelemetryId(EndingType);
	if (TelemetryEndpoint.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Telemetry skipped: endpoint not configured"));
		return;
	}
	if (EndingId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Telemetry skipped: unknown ending type"));
		return;
	}

	FString SessionToken;
	ULoop9BackendAuthSubsystem* Auth = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		Auth = GI->GetSubsystem<ULoop9BackendAuthSubsystem>();
		if (Auth)
		{
			SessionToken = Auth->GetSessionToken();
		}
	}

	if (!SessionToken.IsEmpty())
	{
		DispatchRunFinished(EndingType, TotalResets, TotalAIInteractions, Commitment, SessionToken);
		return;
	}

	if (Auth && Auth->RequiresSteamSession())
	{
		ClearPendingTelemetryAuth();
		bPendingRunFinished = true;
		PendingEndingType = EndingType;
		PendingTotalResets = TotalResets;
		PendingTotalAIInteractions = TotalAIInteractions;
		PendingCommitment = Commitment;
		AuthReadyHandle = Auth->OnSessionReady.AddUObject(this, &ULoop9TelemetrySubsystem::OnTelemetryAuthReady);
		AuthFailedHandle = Auth->OnSessionFailed.AddUObject(this, &ULoop9TelemetrySubsystem::OnTelemetryAuthFailed);

		if (!Auth->EnsureSession() && bPendingRunFinished)
		{
			OnTelemetryAuthFailed(TEXT("authentication is cooling down or unavailable"));
		}
		return;
	}

	if (GameToken.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Telemetry skipped: no session/game token"));
		return;
	}

	DispatchRunFinished(EndingType, TotalResets, TotalAIInteractions, Commitment, FString());
}

void ULoop9TelemetrySubsystem::DispatchRunFinished(
	ELoopEndingType EndingType,
	int32 TotalResets,
	int32 TotalAIInteractions,
	const FDragojloCommitmentState& Commitment,
	const FString& SessionToken)
{
	const FString EndingId = EndingTelemetryId(EndingType);
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(TelemetryEndpoint);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetTimeout(15.0f);

	if (!SessionToken.IsEmpty())
	{
		HttpRequest->SetHeader(TEXT("X-Session-Token"), SessionToken);
	}
	else
	{
		HttpRequest->SetHeader(TEXT("X-Game-Token"), GameToken);
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField(TEXT("ending"), EndingId);
	JsonObject->SetNumberField(TEXT("resets"), FMath::Max(0, TotalResets));
	JsonObject->SetNumberField(TEXT("ai_messages"), FMath::Max(0, TotalAIInteractions));
	JsonObject->SetStringField(TEXT("build"), FApp::GetBuildVersion());
	JsonObject->SetBoolField(TEXT("location_misdirection_used"), Commitment.bLocationMisdirectionUsed);
	JsonObject->SetBoolField(TEXT("visited_suggested_decoy"), Commitment.bVisitedSuggestedDecoy);
	JsonObject->SetBoolField(TEXT("contradiction_exposed"), Commitment.bContradictionExposed);
	// 1.1: the one-shot previous-floor slip, so its chance can be tuned from data.
	JsonObject->SetBoolField(TEXT("stale_floor_used"), Commitment.bStaleFloorUsed);
	JsonObject->SetNumberField(TEXT("lift_advice_count"), FMath::Max(0, Commitment.LiftAdviceCount));
	JsonObject->SetNumberField(TEXT("followed_lift_advice_count"), FMath::Max(0, Commitment.FollowedLiftAdviceCount));
	JsonObject->SetNumberField(TEXT("wrong_lift_advice_count"), FMath::Max(0, Commitment.WrongLiftAdviceCount));
	JsonObject->SetNumberField(TEXT("followed_wrong_lift_advice_count"), FMath::Max(0, Commitment.FollowedWrongLiftAdviceCount));
	if (Commitment.DecoyVisitSeconds >= 0.0f)
	{
		JsonObject->SetNumberField(TEXT("decoy_visit_seconds"), Commitment.DecoyVisitSeconds);
	}

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(Body);

	UE_LOG(LogTemp, Log, TEXT("Telemetry POST %s ending=%s resets=%d ai_messages=%d"),
		*TelemetryEndpoint, *EndingId, TotalResets, TotalAIInteractions);

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			const int32 Code = Response.IsValid() ? Response->GetResponseCode() : 0;
			const bool bAccepted = bWasSuccessful && Response.IsValid() && Code >= 200 && Code < 300;
			UE_LOG(LogTemp, Log, TEXT("Run telemetry result: %s (HTTP %d)"),
				bAccepted ? TEXT("ok") : TEXT("failed"), Code);
		});

	if (!HttpRequest->ProcessRequest())
	{
		UE_LOG(LogTemp, Warning, TEXT("Run telemetry request could not be started"));
	}
}

void ULoop9TelemetrySubsystem::ClearPendingTelemetryAuth()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoop9BackendAuthSubsystem* Auth = GI->GetSubsystem<ULoop9BackendAuthSubsystem>())
		{
			if (AuthReadyHandle.IsValid())
			{
				Auth->OnSessionReady.Remove(AuthReadyHandle);
			}
			if (AuthFailedHandle.IsValid())
			{
				Auth->OnSessionFailed.Remove(AuthFailedHandle);
			}
		}
	}

	AuthReadyHandle.Reset();
	AuthFailedHandle.Reset();
	bPendingRunFinished = false;
}

void ULoop9TelemetrySubsystem::OnTelemetryAuthReady()
{
	if (!bPendingRunFinished)
	{
		return;
	}

	const ELoopEndingType EndingType = PendingEndingType;
	const int32 TotalResets = PendingTotalResets;
	const int32 TotalAIInteractions = PendingTotalAIInteractions;
	const FDragojloCommitmentState Commitment = PendingCommitment;
	ClearPendingTelemetryAuth();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoop9BackendAuthSubsystem* Auth = GI->GetSubsystem<ULoop9BackendAuthSubsystem>())
		{
			const FString SessionToken = Auth->GetSessionToken();
			if (!SessionToken.IsEmpty())
			{
				DispatchRunFinished(EndingType, TotalResets, TotalAIInteractions, Commitment, SessionToken);
				return;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Telemetry skipped: auth completed without a valid session"));
}

void ULoop9TelemetrySubsystem::OnTelemetryAuthFailed(const FString& Reason)
{
	if (!bPendingRunFinished)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Telemetry skipped: Steam authentication failed (%s)"), *Reason);
	ClearPendingTelemetryAuth();
}
