#include "Subsystems/Loop9TelemetrySubsystem.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/App.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/Loop9BackendAuthSubsystem.h"

void ULoop9TelemetrySubsystem::ConfigureFromChatEndpoint(const FString& ChatEndpoint, const FString& InGameToken)
{
	FString Derived = ChatEndpoint;
	if (Derived.EndsWith(TEXT("/api/chat")))
	{
		Derived = Derived.LeftChop(9) + TEXT("/api/telemetry/run");
	}
	else
	{
		Derived.Empty();
	}

	TelemetryEndpoint = Derived;
	GameToken = InGameToken;
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
	default: return FString();
	}
}

void ULoop9TelemetrySubsystem::SendRunFinished(ELoopEndingType EndingType, int32 TotalResets, int32 TotalAIInteractions)
{
	const FString EndingId = EndingTelemetryId(EndingType);
	if (TelemetryEndpoint.IsEmpty() || EndingId.IsEmpty())
	{
		return;
	}

	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(TelemetryEndpoint);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetTimeout(15.0f);

	FString SessionToken;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (const ULoop9BackendAuthSubsystem* Auth = GI->GetSubsystem<ULoop9BackendAuthSubsystem>())
		{
			SessionToken = Auth->GetSessionToken();
		}
	}

	if (!SessionToken.IsEmpty())
	{
		HttpRequest->SetHeader(TEXT("X-Session-Token"), SessionToken);
	}
	else if (!GameToken.IsEmpty())
	{
		HttpRequest->SetHeader(TEXT("X-Game-Token"), GameToken);
	}
	else
	{
		return;
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField(TEXT("ending"), EndingId);
	JsonObject->SetNumberField(TEXT("resets"), FMath::Max(0, TotalResets));
	JsonObject->SetNumberField(TEXT("ai_messages"), FMath::Max(0, TotalAIInteractions));
	JsonObject->SetStringField(TEXT("build"), FApp::GetBuildVersion());

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(Body);

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			// Fire-and-forget by design; log only for debugging.
			UE_LOG(LogTemp, Verbose, TEXT("Run telemetry sent: %s (HTTP %d)"),
				bWasSuccessful ? TEXT("ok") : TEXT("failed"),
				Response.IsValid() ? Response->GetResponseCode() : 0);
		});

	HttpRequest->ProcessRequest();
}
