#include "AI_Friend.h"
#include "AI_ChatWidget.h"
#include "AI/Services/Loop9BackendChatService.h"
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
			ChatWidget->AddMessageToChat(LimitReply, false, true);
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

	ULoopManagerSubsystem* LoopManager = nullptr;
	UAnomalyManager* AnomalyManager = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>();
		AnomalyManager = GI->GetSubsystem<UAnomalyManager>();
	}

	ULoop9BackendAuthSubsystem* AuthSubsystem = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		AuthSubsystem = GI->GetSubsystem<ULoop9BackendAuthSubsystem>();
	}
	if (AuthSubsystem)
	{
		AuthSubsystem->EnsureSession();
	}

	const int32 LoopIndex = LoopManager ? FMath::Max(1, LoopManager->CurrentLoop) : 1;
	FLoop9ChatRequestContext RequestContext;
	RequestContext.Message = Message;
	RequestContext.APIEndpoint = APIEndpoint;
	RequestContext.GameToken = GameToken;
	RequestContext.SessionToken = AuthSubsystem ? AuthSubsystem->GetSessionToken() : FString();
	RequestContext.PlayerId = PlayerId;
	RequestContext.PreferredLanguage = PreferredLanguage;
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

	ULoop9BackendChatService* ChatService = NewObject<ULoop9BackendChatService>(this);
	ChatService->SendChatRequest(RequestContext, FOnLoop9ChatResponseReceived::CreateWeakLambda(this,
		[this](const FLoop9ChatResponse& ChatResponse)
		{
			if (!ChatResponse.bSuccess)
			{
				// A 403 with an active session usually means the token expired
				// server-side; drop it so the next message re-authenticates.
				if (ChatResponse.HttpCode == 403)
				{
					if (UGameInstance* GI = GetGameInstance())
					{
						if (ULoop9BackendAuthSubsystem* Auth = GI->GetSubsystem<ULoop9BackendAuthSubsystem>())
						{
							Auth->InvalidateAndReauth();
						}
					}
				}

				LastAIResponse = ChatResponse.ErrorMessage;
				OnResponseReceived(LastAIResponse);
				return;
			}

			if (UGameInstance* GI = GetGameInstance())
			{
				if (ULoopManagerSubsystem* LoopMgr = GI->GetSubsystem<ULoopManagerSubsystem>())
				{
					LoopMgr->ApplyAIDiagnosedKindnessDelta(ChatResponse.KindnessDelta);
					LoopMgr->ApplyAIDiagnosedSuspicionDelta(ChatResponse.SuspicionDelta);
				}
			}

			LastAIResponse = ChatResponse.Reply;
			if (UAI_ChatWidget* ChatWidget = GetChatWidgetTyped())
			{
				ChatWidget->AddMessageToChat(ChatResponse.Reply, false, ShouldUseAnomalyMumble());
			}

			OnResponseReceived(ChatResponse.Reply);
		}));

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
			ChatWidget->AddMessageToChat(InitialRuleMessage, false, ShouldUseAnomalyMumble());
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
