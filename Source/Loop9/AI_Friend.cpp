#include "AI_Friend.h"
#include "AI_ChatWidget.h"
#include "AI/Services/Loop9BackendChatService.h"
#include "AI/Services/Loop9BackendEndpointUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Subsystems/RelationshipSubsystem.h"
#include "Subsystems/AnomalyManager.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Subsystems/Loop9GameplayNotificationSubsystem.h"
#include "Subsystems/Loop9BackendAuthSubsystem.h"
#include "Subsystems/Loop9TelemetrySubsystem.h"
#include "Internationalization/Culture.h"
#include "Misc/Guid.h"

AAI_Friend::AAI_Friend()
{
	// Ring / loop-change work runs on timers and interaction callbacks — no idle tick.
	PrimaryActorTick.bCanEverTick = false;

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

	FString LoadedPlayerId;
	if (GConfig->GetString(TEXT("/Script/Loop9.AI_Friend"), TEXT("PlayerId"), LoadedPlayerId, GGameIni) && !LoadedPlayerId.IsEmpty())
	{
		PlayerId = LoadedPlayerId;
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

	EnsureStablePlayerId();
}

void AAI_Friend::EnsureStablePlayerId()
{
	PlayerId = PlayerId.TrimStartAndEnd();
	if (!PlayerId.IsEmpty())
	{
		return;
	}

	PlayerId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	GConfig->SetString(TEXT("/Script/Loop9.AI_Friend"), TEXT("PlayerId"), *PlayerId, GGameIni);
	GConfig->Flush(false, GGameIni);
	UE_LOG(LogTemp, Log, TEXT("AI_Friend generated stable PlayerId=%s"), *PlayerId);
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

bool AAI_Friend::ShouldUseAnomalyMumble() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	const UAnomalyManager* AnomalyManager = GameInstance->GetSubsystem<UAnomalyManager>();
	return AnomalyManager && AnomalyManager->GetActiveAnomalyCount() > 0;
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

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoop9BackendAuthSubsystem* AuthSubsystem = GI->GetSubsystem<ULoop9BackendAuthSubsystem>())
		{
			AuthSubsystem->ConfigureFromChatEndpoint(APIEndpoint);
		}

		if (ULoop9TelemetrySubsystem* Telemetry = GI->GetSubsystem<ULoop9TelemetrySubsystem>())
		{
			Telemetry->ConfigureFromChatEndpoint(APIEndpoint, GameToken);
		}
	}

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

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>())
		{
			LoopManager->RegisterAIFriend(this);
		}
	}

	StartInitialRingIfNeeded();
}

void AAI_Friend::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopInitialRing();
	ClearPendingAuthChat();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.RemoveDynamic(this, &AAI_Friend::OnTriggerBeginOverlap);
		TriggerBox->OnComponentEndOverlap.RemoveDynamic(this, &AAI_Friend::OnTriggerEndOverlap);
	}

	if (bInteractionInputCaptured && CurrentPlayerController)
	{
		SetPlayerMovementEnabled(CurrentPlayerController, true);
		ApplyInteractionInputMode(CurrentPlayerController, false);
		bInteractionInputCaptured = false;
	}

	if (ChatWidgetInstance)
	{
		if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
		{
			ChatWidget->HideThinkingIndicator();
		}
		ChatWidgetInstance->RemoveFromParent();
		ChatWidgetInstance = nullptr;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>())
		{
			LoopManager->UnregisterAIFriend(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AAI_Friend::HandleLoopChanged()
{
	// Ignore responses and pending auth callbacks that belong to the previous loop/run.
	++ChatRequestGeneration;
	ClearPendingAuthChat();
	if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
	{
		ChatWidget->HideThinkingIndicator();
	}

	if (ShouldStopInitialRingForLoopChange())
	{
		StopInitialRing();
	}
}

void AAI_Friend::ClearPendingAuthChat()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PendingAuthTimeoutHandle);
	}

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
	PendingAuthChatMessage.Reset();
	bPendingAuthChat = false;
	bPendingAuthChatIsRetry = false;
}

void AAI_Friend::OnAuthSessionReadyForPendingChat()
{
	if (!bPendingAuthChat)
	{
		return;
	}

	const FString Message = PendingAuthChatMessage;
	const bool bIsAuthRetry = bPendingAuthChatIsRetry;
	ClearPendingAuthChat();
	DispatchChatRequest(Message, bIsAuthRetry);
}

void AAI_Friend::OnAuthSessionFailedForPendingChat(const FString& Reason)
{
	if (!bPendingAuthChat)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("AI_Friend: pending chat aborted — auth failed (%s)"), *Reason);
	ClearPendingAuthChat();
	MessagesSentThisLoop = FMath::Max(0, MessagesSentThisLoop - 1);
	HandleLocalizedChatFailure(0);
}

bool AAI_Friend::QueuePendingAuthChat(const FString& Message, bool bIsAuthRetry, bool bForceReauth)
{
	if (bPendingAuthChat)
	{
		return false;
	}

	ULoop9BackendAuthSubsystem* AuthSubsystem = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		AuthSubsystem = GI->GetSubsystem<ULoop9BackendAuthSubsystem>();
	}
	if (!AuthSubsystem)
	{
		return false;
	}

	PendingAuthChatMessage = Message;
	bPendingAuthChat = true;
	bPendingAuthChatIsRetry = bIsAuthRetry;
	AuthReadyHandle = AuthSubsystem->OnSessionReady.AddUObject(this, &AAI_Friend::OnAuthSessionReadyForPendingChat);
	AuthFailedHandle = AuthSubsystem->OnSessionFailed.AddUObject(this, &AAI_Friend::OnAuthSessionFailedForPendingChat);

	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<AAI_Friend> WeakThis(this);
		World->GetTimerManager().SetTimer(
			PendingAuthTimeoutHandle,
			[WeakThis]()
			{
				if (WeakThis.IsValid() && WeakThis->bPendingAuthChat)
				{
					WeakThis->OnAuthSessionFailedForPendingChat(TEXT("chat auth wait timed out"));
				}
			},
			17.0f,
			false);
	}

	const bool bAuthAttemptActive = bForceReauth
		? AuthSubsystem->InvalidateAndReauth()
		: AuthSubsystem->EnsureSession();
	if (!bAuthAttemptActive && bPendingAuthChat)
	{
		OnAuthSessionFailedForPendingChat(TEXT("Steam authentication is cooling down or unavailable"));
	}

	return bPendingAuthChat;
}

void AAI_Friend::HandleLocalizedChatFailure(int32 HttpCode)
{
	FString UserFacingReply;
	if (HttpCode == 0)
	{
		UserFacingReply = NSLOCTEXT("Loop9Chat", "ChatConnectionLost",
			"...the line crackles and goes dead. (No connection. Check your internet and try again.)").ToString();
	}
	else if (HttpCode == 429)
	{
		UserFacingReply = NSLOCTEXT("Loop9Chat", "ChatRateLimited",
			"...the line is busy. He cannot take more calls right now. Try again later.").ToString();
	}
	else
	{
		UserFacingReply = NSLOCTEXT("Loop9Chat", "ChatServerError",
			"...static hisses through the receiver. Something is wrong on the other side. Try again.").ToString();
	}

	LastAIResponse = UserFacingReply;
	if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
	{
		ChatWidget->AddMessageToChat(UserFacingReply, false, true);
	}

	OnResponseReceived(UserFacingReply);
}

FString AAI_Friend::SayToAI(const FString& Message)
{
	RefreshLoopMessageLimitCounter();

	if (MaxMessagesPerLoop > 0 && MessagesSentThisLoop >= MaxMessagesPerLoop)
	{
		const FString LimitReply = bUseSignalDropOnLimit
			? BuildSignalDropMessage()
			: NSLOCTEXT("Loop9Chat", "LowSignalReply", "Low signal. Try again later.").ToString();

		LastAIResponse = LimitReply;

		if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
		{
			ChatWidget->AddMessageToChat(LimitReply, false, true);
		}

		OnResponseReceived(LimitReply);
		return TEXT("Blocked: message limit reached for this loop");
	}

	if (APIEndpoint.IsEmpty())
	{
		return TEXT("Error: API Endpoint not configured");
	}

	if (bPendingAuthChat)
	{
		return TEXT("Request pending auth...");
	}

	MessagesSentThisLoop++;

	ULoop9BackendAuthSubsystem* AuthSubsystem = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		AuthSubsystem = GI->GetSubsystem<ULoop9BackendAuthSubsystem>();
	}

	const bool bSteamAuthRequired = AuthSubsystem && AuthSubsystem->RequiresSteamSession();
	const bool bHasSession = AuthSubsystem && AuthSubsystem->HasValidSession();

	if (bSteamAuthRequired && !bHasSession)
	{
		// Authorize then dispatch — never send an unauthenticated Steam production request.
		if (!QueuePendingAuthChat(Message, false, false))
		{
			return TEXT("Error: Steam authentication failed");
		}

		if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
		{
			ChatWidget->ShowThinkingIndicator();
		}

		UE_LOG(LogTemp, Log, TEXT("AI request queued until Steam session is ready. MsgLen=%d"), Message.Len());
		return TEXT("Request queued (awaiting Steam auth)...");
	}

	DispatchChatRequest(Message);
	return TEXT("Request sent... (async)");
}

void AAI_Friend::DispatchChatRequest(const FString& Message, bool bIsAuthRetry)
{
	UE_LOG(LogTemp, Log, TEXT("AI request start. Endpoint=%s | Mode=BackendContract | MsgLen=%d"), *APIEndpoint, Message.Len());
	if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
	{
		ChatWidget->ShowThinkingIndicator();
	}

	ULoopManagerSubsystem* LoopManager = nullptr;
	UAnomalyManager* AnomalyManager = nullptr;
	ULoop9BackendAuthSubsystem* AuthSubsystem = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>();
		AnomalyManager = GI->GetSubsystem<UAnomalyManager>();
		AuthSubsystem = GI->GetSubsystem<ULoop9BackendAuthSubsystem>();
	}

	const FString SessionToken = AuthSubsystem ? AuthSubsystem->GetSessionToken() : FString();
	const int32 LoopIndex = LoopManager ? FMath::Clamp(LoopManager->CurrentLoop, 1, 9) : 1;

	FLoop9ChatRequestContext RequestContext;
	RequestContext.RequestId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	RequestContext.Message = Message;
	RequestContext.APIEndpoint = Loop9BackendEndpointUtils::DeriveChatEndpoint(APIEndpoint);
	RequestContext.GameToken = GameToken;
	RequestContext.SessionToken = SessionToken;
	RequestContext.PlayerId = PlayerId;
	// Explicit config/env override wins; otherwise the AI follows the UI language.
	RequestContext.PreferredLanguage = !PreferredLanguage.IsEmpty()
		? PreferredLanguage
		: FInternationalization::Get().GetCurrentCulture()->GetTwoLetterISOLanguageName();
	RequestContext.AIStability = LoopManager ? LoopManager->GetAIStability() : 1.0f;
	RequestContext.LoopIndex = LoopIndex;
	RequestContext.Trust = LoopManager ? LoopManager->GetTrust() : 0.5f;
	RequestContext.Kindness = LoopManager ? LoopManager->GetKindness() : 0.5f;
	RequestContext.Suspicion = LoopManager ? LoopManager->GetSuspicion() : 0.2f;
	RequestContext.Dependency = LoopManager ? LoopManager->GetDependency() : 0.2f;
	RequestContext.AnomalyContext = TEXT("No active anomaly currently detected.");
	RequestContext.AnomalyKey = TEXT("none");

	if (AnomalyManager)
	{
		AnomalyManager->UpdateLoopAnomalyTracking(LoopIndex);
		RequestContext.AnomalyContext = AnomalyManager->GetCurrentLoopAnomalyContext();
		RequestContext.bRepeatAnomaly = AnomalyManager->IsCurrentLoopAnomalyRepeat();
		RequestContext.AnomalyKey = AnomalyManager->GetCurrentLoopAnomalyKey();
	}

	const bool bUsedSessionToken = !SessionToken.IsEmpty();
	const uint64 RequestGeneration = ChatRequestGeneration;

	ULoop9BackendChatService* ChatService = NewObject<ULoop9BackendChatService>(this);
	ChatService->SendChatRequest(RequestContext, FOnLoop9ChatResponseReceived::CreateWeakLambda(this,
		[this, Message, bUsedSessionToken, bIsAuthRetry, RequestGeneration, LoopIndex](const FLoop9ChatResponse& ChatResponse)
		{
			if (RequestGeneration != ChatRequestGeneration || ResolveCurrentLoopIndex() != LoopIndex)
			{
				UE_LOG(LogTemp, Log, TEXT("Ignoring stale AI response from loop %d."), LoopIndex);
				return;
			}

			if (!ChatResponse.bSuccess)
			{
				// Authentication runs before quotas/provider dispatch, so one retry
				// after refreshing a rejected session cannot duplicate AI spend.
				if (ChatResponse.HttpCode == 403 && bUsedSessionToken && !bIsAuthRetry)
				{
					if (QueuePendingAuthChat(Message, true, true))
					{
						UE_LOG(LogTemp, Log, TEXT("AI request waiting for one-time reauthentication retry."));
					}
					return;
				}

				UE_LOG(LogTemp, Warning, TEXT("AI request failed. HttpCode=%d | %s"),
					ChatResponse.HttpCode, *ChatResponse.ErrorMessage);

				MessagesSentThisLoop = FMath::Max(0, MessagesSentThisLoop - 1);
				HandleLocalizedChatFailure(ChatResponse.HttpCode);
				return;
			}

			if (UGameInstance* GI = GetGameInstance())
			{
				if (ULoopManagerSubsystem* LoopMgr = GI->GetSubsystem<ULoopManagerSubsystem>())
				{
					LoopMgr->ApplyAIDiagnosedKindnessDelta(ChatResponse.KindnessDelta);
					LoopMgr->ApplyAIDiagnosedSuspicionDelta(ChatResponse.SuspicionDelta);
					// Count successful validated replies only (achievements / telemetry).
					LoopMgr->RegisterAIInteraction();
				}
			}

			LastAIResponse = ChatResponse.Reply;
			if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
			{
				ChatWidget->AddMessageToChat(ChatResponse.Reply, false, ShouldUseAnomalyMumble());
			}

			OnResponseReceived(ChatResponse.Reply);
		}));
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

	if (ShouldStopInitialRingForLoopChange())
	{
		StopInitialRing();
		return;
	}

	if (RingsPlayedCount >= InitialRingRepeatCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(InitialRingTimerHandle);
		return;
	}

	if (RingsPlayedCount == 0 && bShowInitialRingNotification && !InitialRingNotificationText.IsEmpty())
	{
		if (ULoop9GameplayNotificationSubsystem* Notifications = ULoop9GameplayNotificationSubsystem::GetGameplayNotifications(this))
		{
			Notifications->AddMessage(InitialRingNotificationText, InitialRingNotificationDuration);
		}
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

	if (PhoneAnswerSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			PhoneAnswerSound,
			GetActorLocation(),
			GetActorRotation(),
			1.0f,
			1.0f,
			0.0f,
			PhoneAnswerAttenuationSettings,
			nullptr);
	}
	else if (PhoneInteractSound)
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
			// Resolve the localized default at use time so a language switch
			// mid-session still produces the right text.
			const FString RuleMessage = InitialRuleMessage.IsEmpty()
				? NSLOCTEXT("Loop9Chat", "InitialRuleMessage",
					"Listen carefully: if you notice any irregularity, take the elevator that has interior light on (RESTART). If you find no anomaly, take the elevator without interior light (NEXT). The first loop is clean, so take your time and learn the baseline.").ToString()
				: InitialRuleMessage;

			ChatWidget->AddMessageToChat(RuleMessage, false, ShouldUseAnomalyMumble());
			bInitialMessageInjected = true;
		}

		// Phantom message anomaly: a "player" message the player never sent.
		if (ChatWidget && !PendingPhantomMessage.IsEmpty() && !bPhantomMessageShown)
		{
			ChatWidget->AddMessageToChat(PendingPhantomMessage, true);
			bPhantomMessageShown = true;
		}

		SetPlayerMovementEnabled(PlayerController, false);
		ApplyInteractionInputMode(PlayerController, true);
		bInteractionInputCaptured = true;
	}
}

void AAI_Friend::QueuePhantomPlayerMessage(const FString& Message)
{
	PendingPhantomMessage = Message;
	bPhantomMessageShown = false;

	// If the chat is already open, inject immediately.
	if (ChatWidgetInstance && ChatWidgetInstance->IsInViewport())
	{
		if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
		{
			ChatWidget->AddMessageToChat(PendingPhantomMessage, true);
			bPhantomMessageShown = true;
		}
	}
}

void AAI_Friend::ClearPhantomPlayerMessage()
{
	PendingPhantomMessage.Empty();
	bPhantomMessageShown = false;
}

bool AAI_Friend::TryInteract_Implementation(APlayerController* InteractingController)
{
	OpenChatWidget(InteractingController);
	return true;
}

FText AAI_Friend::GetInteractionPromptText_Implementation() const
{
	return NSLOCTEXT("Loop9Interaction", "AnswerPhone", "Answer");
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
		bInteractionInputCaptured = false;
	}
}
