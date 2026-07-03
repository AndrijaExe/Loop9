#include "AI_Friend.h"
#include "AI_ChatWidget.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Subsystems/AnomalyManager.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

namespace
{
  FString SanitizeAIReplyText(const FString& InText)
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

	bool TryExtractStateDeltas(const FString& RawContent, FString& OutReply, int32& OutKindnessDelta, int32& OutSuspicionDelta)
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
				FString ValuePart = MetaPart.RightChop(9).TrimStartAndEnd();
				ParsedK = FCString::Atoi(*ValuePart);
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
			OutSuspicionDelta = 0;
			return true;
		}

		FString ReplyPart = RawContent.Left(MarkerIndex).TrimStartAndEnd();
		FString MetaPart = RawContent.Mid(MarkerIndex + 7).TrimStartAndEnd();

		int32 ParsedK = 0;
		int32 ParsedS = 0;
		bool bHasK = TryParseIntAfterToken(MetaPart, TEXT("KINDNESS="), ParsedK) || TryParseIntAfterToken(MetaPart, TEXT("KINDNESS:"), ParsedK);
		bool bHasS = TryParseIntAfterToken(MetaPart, TEXT("SUSPICION="), ParsedS) || TryParseIntAfterToken(MetaPart, TEXT("SUSPICION:"), ParsedS);

		if (!bHasK && !bHasS)
		{
			return false;
		}

		OutReply = ReplyPart.IsEmpty() ? RawContent : ReplyPart;
		OutKindnessDelta = FMath::Clamp(ParsedK, -1, 1);
		OutSuspicionDelta = FMath::Clamp(ParsedS, -1, 1);
		return true;

	}
}

AAI_Friend::AAI_Friend()
{
	PrimaryActorTick.bCanEverTick = true;

	PhoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhoneMesh"));
	RootComponent = PhoneMesh;
    PhoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(PhoneMesh);
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

int32 AAI_Friend::ResolveCurrentLoopIndex() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const ULoopManagerSubsystem* LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>())
		{
			return FMath::Max(1, LoopManager->CurrentLoop);
		}
	}

	return 1;
}

void AAI_Friend::RefreshLoopMessageLimitCounter()
{
	const int32 CurrentLoopIndex = ResolveCurrentLoopIndex();
	if (LastLoopIndexForMessageLimit != CurrentLoopIndex)
	{
		LastLoopIndexForMessageLimit = CurrentLoopIndex;
		MessagesSentThisLoop = 0;
	}
}

void AAI_Friend::LoadConfiguredOverrides()
{
	FString LoadedGameToken;
	if (GConfig->GetString(TEXT("/Script/Loop9.AI_Friend"), TEXT("GameToken"), LoadedGameToken, GGameIni) && !LoadedGameToken.IsEmpty())
	{
		GameToken = LoadedGameToken;
	}

	FString LoadedLanguage;
	if (GConfig->GetString(TEXT("/Script/Loop9.AI_Friend"), TEXT("PreferredLanguage"), LoadedLanguage, GGameIni) && !LoadedLanguage.IsEmpty())
	{
		PreferredLanguage = LoadedLanguage;
	}

	FString EnvEndpoint = FPlatformMisc::GetEnvironmentVariable(TEXT("APIEndpoint"));
	if (!EnvEndpoint.IsEmpty())
	{
		APIEndpoint = EnvEndpoint;
	}

	FString EnvGameToken = FPlatformMisc::GetEnvironmentVariable(TEXT("LOOP9_GAME_TOKEN"));
	if (!EnvGameToken.IsEmpty())
	{
		GameToken = EnvGameToken;
	}

	FString EnvPlayerId = FPlatformMisc::GetEnvironmentVariable(TEXT("Player_Id"));
	if (!EnvPlayerId.IsEmpty())
	{
		PlayerId = EnvPlayerId;
	}

	FString EnvPreferredLanguage = FPlatformMisc::GetEnvironmentVariable(TEXT("LOOP9_PREFERRED_LANGUAGE"));
	if (!EnvPreferredLanguage.IsEmpty())
	{
		PreferredLanguage = EnvPreferredLanguage;
	}

	if (APIEndpoint.Equals(TEXT("http:"), ESearchCase::IgnoreCase)
		|| APIEndpoint.Equals(TEXT("https:"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Warning, TEXT("AI_Friend endpoint appears truncated (%s). Falling back to local backend endpoint."), *APIEndpoint);
		APIEndpoint = TEXT("https://loop9-backend.onrender.com/api/chat");
	}
}

void AAI_Friend::StopInitialRing()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialRingTimerHandle);
	}

	if (ActiveInitialRingAudioComponent)
	{
		ActiveInitialRingAudioComponent->Stop();
		ActiveInitialRingAudioComponent->DestroyComponent();
		ActiveInitialRingAudioComponent = nullptr;
	}
}

void AAI_Friend::StartInitialRingIfNeeded()
{
	if (!bAutoRingOnBeginPlay || !InitialRingSound || !GetWorld())
	{
		return;
	}

	const float RingDelay = InitialRingInterval > 0.0f
		? InitialRingInterval
		: FMath::Max(0.05f, InitialRingSound->GetDuration());

	GetWorld()->GetTimerManager().SetTimer(InitialRingTimerHandle, this, &AAI_Friend::TriggerInitialRing, RingDelay, true, 5.0f);
}

bool AAI_Friend::ShouldStopInitialRingForLoopChange() const
{
	return !bInitialMessageInjected && ResolveCurrentLoopIndex() != InitialLoopNumberAtBeginPlay;
}

UAI_ChatWidget* AAI_Friend::GetChatWidgetTyped() const
{
	return Cast<UAI_ChatWidget>(ChatWidgetInstance);
}

UAI_ChatWidget* AAI_Friend::GetOrCreateChatWidgetTyped(APlayerController* PlayerController)
{
	if (!PlayerController || !ChatWidgetClass)
	{
		return nullptr;
	}

	if (!ChatWidgetInstance)
	{
		ChatWidgetInstance = CreateWidget<UUserWidget>(PlayerController, ChatWidgetClass);
}

	if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
	{
		ChatWidget->AIFriendRef = this;
		return ChatWidget;
	}

	return nullptr;
}

void AAI_Friend::SetPlayerMovementEnabled(APlayerController* PlayerController, bool bEnabled) const
{
	if (!PlayerController)
	{
		return;
	}

	if (ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerController->GetPawn()))
	{
		if (UCharacterMovementComponent* MovementComp = PlayerCharacter->GetCharacterMovement())
		{
			if (bEnabled)
			{
				MovementComp->SetMovementMode(MOVE_Walking);
			}
			else
			{
				MovementComp->DisableMovement();
			}
		}
	}
}

void AAI_Friend::ApplyInteractionInputMode(APlayerController* PlayerController, bool bUIOnly) const
{
	if (!PlayerController)
	{
		return;
	}

	if (bUIOnly)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
	else
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
	}
}

void AAI_Friend::BeginPlay()
{
	Super::BeginPlay();

	bInitialMessageInjected = false;
	RingsPlayedCount = 0;
	InitialLoopNumberAtBeginPlay = ResolveCurrentLoopIndex();

	LoadConfiguredOverrides();

	UE_LOG(LogTemp, Log, TEXT("AI_Friend configured. Endpoint=%s | BackendTokenSet=%s"), *APIEndpoint, GameToken.IsEmpty() ? TEXT("NO") : TEXT("YES"));

	if (TriggerBox)
	{
		if (!TriggerBox->OnComponentBeginOverlap.IsAlreadyBound(this, &AAI_Friend::OnTriggerBeginOverlap))
		{
			TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AAI_Friend::OnTriggerBeginOverlap);
		}

		if (!TriggerBox->OnComponentEndOverlap.IsAlreadyBound(this, &AAI_Friend::OnTriggerEndOverlap))
		{
			TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AAI_Friend::OnTriggerEndOverlap);
		}
	}

	StartInitialRingIfNeeded();
}

void AAI_Friend::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ShouldStopInitialRingForLoopChange())
	{
		StopInitialRing();
	}

}

FString AAI_Friend::SayToAI(const FString& Message)
{
	const int32 CurrentLoopIndex = ResolveCurrentLoopIndex();
	RefreshLoopMessageLimitCounter();

	if (MaxMessagesPerLoop > 0 && MessagesSentThisLoop >= MaxMessagesPerLoop)
	{
		const FString LimitReply = bUseSignalDropOnLimit
			? BuildSignalDropMessage()
			: TEXT("Low signal. Try again later.");

		LastAIResponse = LimitReply;

		if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
		{
			ChatWidget->AddMessageToChat(LimitReply, false);
		}

		OnResponseReceived(LimitReply);
		return TEXT("Blocked: message limit reached for this loop");
	}

	MessagesSentThisLoop++;

	if (APIEndpoint.IsEmpty())
	{
		return TEXT("Error: API Endpoint not configured");
	}

    UE_LOG(LogTemp, Log, TEXT("AI request start. Endpoint=%s | Mode=BackendContract | MsgLen=%d"), *APIEndpoint, Message.Len());

	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(APIEndpoint);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

   if (!GameToken.IsEmpty())
	{
		HttpRequest->SetHeader(TEXT("X-Game-Token"), GameToken);
	}
   else
	{
        UE_LOG(LogTemp, Warning, TEXT("Backend contract mode active but GameToken is empty."));
	}

   if (!PlayerId.IsEmpty())
   {
	   HttpRequest->AppendToHeader(TEXT("X-Player-Id"), PlayerId);
   }
   else
   {
	   UE_LOG(LogTemp, Warning, TEXT("Player id is not set properly"));
   }

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField(TEXT("message"), Message);

	ULoopManagerSubsystem* LoopManager = nullptr;
	UAnomalyManager* AnomalyManager = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>();
		AnomalyManager = GI->GetSubsystem<UAnomalyManager>();
	}

	const float Stability = LoopManager ? FMath::Clamp(LoopManager->AI_Stability, 0.0f, 1.0f) : 1.0f;
	const int32 LoopIndex = LoopManager ? FMath::Max(1, LoopManager->CurrentLoop) : 1;
	FString AnomalyContext = TEXT("No active anomaly currently detected.");
	bool bRepeatAnomalyFromPreviousLoop = false;
	FString AnomalyKey = TEXT("none");
	if (AnomalyManager)
	{
		AnomalyManager->UpdateLoopAnomalyTracking(LoopIndex);
		AnomalyContext = AnomalyManager->GetCurrentLoopAnomalyContext();
		bRepeatAnomalyFromPreviousLoop = AnomalyManager->IsCurrentLoopAnomalyRepeat();
		AnomalyKey = AnomalyManager->GetCurrentLoopAnomalyKey();
	}

	JsonObject->SetStringField(TEXT("language"), PreferredLanguage);
	JsonObject->SetNumberField(TEXT("ai_stability"), Stability);
	JsonObject->SetNumberField(TEXT("loop_index"), LoopIndex);
 JsonObject->SetStringField(TEXT("anomaly_context"),
		bRepeatAnomalyFromPreviousLoop
			? FString::Printf(TEXT("%s Repeat anomaly from previous loop."), *AnomalyContext)
			: AnomalyContext);
	JsonObject->SetBoolField(TEXT("offtopic"), false);

	TSharedPtr<FJsonObject> StateObject = MakeShareable(new FJsonObject());
	if (LoopManager)
	{
		const int32 KindnessDiscrete = (LoopManager->Kindness > 0.66f) ? 1 : ((LoopManager->Kindness < 0.34f) ? -1 : 0);
		const int32 SuspicionDiscrete = (LoopManager->Suspicion > 0.66f) ? 1 : ((LoopManager->Suspicion < 0.34f) ? -1 : 0);

		StateObject->SetNumberField(TEXT("kindness"), KindnessDiscrete);
		StateObject->SetNumberField(TEXT("suspicion"), SuspicionDiscrete);
       StateObject->SetNumberField(TEXT("dependency"), FMath::Clamp(LoopManager->Dependency, 0.0f, 1.0f));
		StateObject->SetNumberField(TEXT("player_confidence"), FMath::Clamp(LoopManager->Trust, 0.0f, 1.0f));
	}
	else
	{
		StateObject->SetNumberField(TEXT("kindness"), 0);
		StateObject->SetNumberField(TEXT("suspicion"), 0);
       StateObject->SetNumberField(TEXT("dependency"), 0.2f);
		StateObject->SetNumberField(TEXT("player_confidence"), 0.5f);
	}

 StateObject->SetBoolField(TEXT("repeat_anomaly"), bRepeatAnomalyFromPreviousLoop);
	StateObject->SetStringField(TEXT("anomaly_key"), AnomalyKey);
	JsonObject->SetObjectField(TEXT("state"), StateObject);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	HttpRequest->SetContentAsString(OutputString);

   HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
            if (bWasSuccessful && Response.IsValid())
			{
				int32 ResponseCode = Response->GetResponseCode();
				FString ResponseString = Response->GetContentAsString();
				UE_LOG(LogTemp, Log, TEXT("AI response received. Code=%d Body=%s"), ResponseCode, *ResponseString);

                if (ResponseCode != 200)
				{
                    LastAIResponse = FString::Printf(TEXT("Error: Backend returned HTTP %d"), ResponseCode);
					OnResponseReceived(LastAIResponse);
					return;
				}

				TSharedPtr<FJsonObject> JsonResponse;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

				if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
				{
                   FString BackendMessage;
					if (JsonResponse->TryGetStringField(TEXT("message"), BackendMessage) && !BackendMessage.IsEmpty())
					{
                     FString CleanReply = BackendMessage;
						int32 KindnessDelta = 0;
						int32 SuspicionDelta = 0;
						if (TryExtractStateDeltas(BackendMessage, CleanReply, KindnessDelta, SuspicionDelta))
						{
                         if (UGameInstance* GI = GetGameInstance())
							{
                              if (ULoopManagerSubsystem* LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>())
								{
                                    LoopManager->ApplyAIDiagnosedKindnessDelta(KindnessDelta);
									LoopManager->ApplyAIDiagnosedSuspicionDelta(SuspicionDelta);
								}
							}
						}

						CleanReply = SanitizeAIReplyText(CleanReply);
						LastAIResponse = CleanReply;

						if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
						{
							ChatWidget->AddMessageToChat(CleanReply, false);
						}

						OnResponseReceived(CleanReply);
						return;
					}
				}

				LastAIResponse = TEXT("Error: Invalid backend response format");
				OnResponseReceived(LastAIResponse);
			}
            else
			{
				LastAIResponse = TEXT("Error: Request failed");
             UE_LOG(LogTemp, Error, TEXT("AI request failed. bWasSuccessful=%s ResponseValid=%s"), bWasSuccessful ? TEXT("true") : TEXT("false"), Response.IsValid() ? TEXT("true") : TEXT("false"));
               OnResponseReceived(LastAIResponse);
			}
		});

	HttpRequest->ProcessRequest();

	return TEXT("Request sent... (async)");
}

FString AAI_Friend::BuildSignalDropMessage() const
{
    auto MakeChunk = [](int32 Len)
	{
      static const TCHAR LocalAlnum[] = TEXT("abcdefghijklmnopqrstuvwxyz0123456789");
		FString Chunk;
		for (int32 i = 0; i < Len; ++i)
		{
         const int32 RandIndex = FMath::RandRange(0, UE_ARRAY_COUNT(LocalAlnum) - 2);
			Chunk.AppendChar(LocalAlnum[RandIndex]);
		}
		return Chunk;
	};

	auto MakeDots = [](int32 Len)
	{
		FString Dots;
		for (int32 i = 0; i < Len; ++i)
		{
			Dots.AppendChar(TEXT('.'));
		}
		return Dots;
	};

	const FString ChunkA = MakeChunk(FMath::RandRange(3, 5));
	const FString ChunkB = MakeChunk(FMath::RandRange(3, 5));
	const FString DotsA = MakeDots(FMath::RandRange(18, 28));
	const FString DotsB = MakeDots(FMath::RandRange(16, 24));

	return FString::Printf(TEXT("%s%s%s%s (LOW SIGNAL)"), *ChunkA, *DotsA, *ChunkB, *DotsB);
}

void AAI_Friend::OnResponseReceived(FString Response)
{
}

void AAI_Friend::TriggerInitialRing()
{
	if (!GetWorld() || !InitialRingSound)
	{
		return;
	}

	if (RingsPlayedCount >= InitialRingRepeatCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(InitialRingTimerHandle);
		return;
	}

    if (ActiveInitialRingAudioComponent)
	{
		ActiveInitialRingAudioComponent->Stop();
		ActiveInitialRingAudioComponent->DestroyComponent();
		ActiveInitialRingAudioComponent = nullptr;
	}

	ActiveInitialRingAudioComponent = UGameplayStatics::SpawnSoundAttached(
		InitialRingSound,
		PhoneMesh ? PhoneMesh : RootComponent,
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		false,
		1.0f,
		1.0f,
		0.0f,
		InitialRingAttenuationSettings,
		nullptr,
		true);

	if (ActiveInitialRingAudioComponent)
	{
		ActiveInitialRingAudioComponent->bAutoDestroy = true;
	}
	RingsPlayedCount++;

	if (RingsPlayedCount >= InitialRingRepeatCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(InitialRingTimerHandle);
	}
}

void AAI_Friend::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && OtherActor == PC->GetPawn())
	{
		bPlayerInRange = true;
		CurrentPlayerController = PC;
	}
}

void AAI_Friend::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && OtherActor == PC->GetPawn())
	{
		bPlayerInRange = false;
		CurrentPlayerController = nullptr;

		if (ChatWidgetInstance && ChatWidgetInstance->IsInViewport())
		{
			CloseChatWidget(PC);
		}
	}
}

void AAI_Friend::OpenChatWidget(APlayerController* PlayerController)
{
	if (!PlayerController || !ChatWidgetClass)
	{
		return;
	}

	if (PhoneInteractSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			PhoneInteractSound,
			GetActorLocation(),
			GetActorRotation(),
			1.0f,
			1.0f,
			0.0f,
			PhoneInteractAttenuationSettings,
			nullptr);
	}

	StopInitialRing();
	UAI_ChatWidget* ChatWidget = GetOrCreateChatWidgetTyped(PlayerController);

	if (ChatWidgetInstance && !ChatWidgetInstance->IsInViewport())
	{
		ChatWidgetInstance->AddToViewport();

		if (ChatWidget && !bInitialMessageInjected)
		{
			ChatWidget->AddMessageToChat(InitialRuleMessage, false);
			bInitialMessageInjected = true;
		}

		SetPlayerMovementEnabled(PlayerController, false);
		ApplyInteractionInputMode(PlayerController, true);
	}
}

bool AAI_Friend::TryInteract_Implementation(APlayerController* InteractingController)
{
	OpenChatWidget(InteractingController);
	return true;
}

FText AAI_Friend::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("Answer"));
}

void AAI_Friend::CloseChatWidget(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	if (ChatWidgetInstance && ChatWidgetInstance->IsInViewport())
	{
		ChatWidgetInstance->RemoveFromParent();
		SetPlayerMovementEnabled(PlayerController, true);
		ApplyInteractionInputMode(PlayerController, false);
	}
}
