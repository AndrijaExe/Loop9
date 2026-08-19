#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Interaction/Loop9Interactable.h"
#include "AI_Friend.generated.h"

UCLASS(Config = Game)
class LOOP9_API AAI_Friend : public AActor, public ILoop9Interactable
{
	GENERATED_BODY()

public:
	AAI_Friend();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PhoneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual bool TryInteract_Implementation(APlayerController* InteractingController) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Limits", meta = (ClampMin = "1"))
	int32 MaxMessagesPerLoop = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Limits")
	bool bUseSignalDropOnLimit = true;

	UFUNCTION(BlueprintCallable, Category = "AI")
	FString SayToAI(const FString& Message);

	/** Called by the loop manager so ringing stops immediately on loop transitions. */
	void HandleLoopChanged();

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString APIEndpoint;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString GameToken;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString PlayerId;

	/**
	 * Explicit AI reply language override (e.g. "sr", "en").
	 * Leave empty to follow the UI language chosen in Settings.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString PreferredLanguage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> ChatWidgetClass;

	/** Optional override for the scripted first message. Leave empty to use the localized default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Scripted")
	FString InitialRuleMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	bool bAutoRingOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring", meta = (ClampMin = "1"))
	int32 InitialRingRepeatCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	TObjectPtr<class USoundBase> InitialRingSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	TObjectPtr<class USoundAttenuation> InitialRingAttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	TObjectPtr<class USoundBase> PhoneInteractSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	TObjectPtr<class USoundAttenuation> PhoneInteractAttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Answer", meta = (DisplayName = "Answer Phone Sound"))
	TObjectPtr<class USoundBase> PhoneAnswerSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Answer", meta = (DisplayName = "Answer Phone Attenuation"))
	TObjectPtr<class USoundAttenuation> PhoneAnswerAttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring", meta = (ClampMin = "0.05"))
	float InitialRingInterval = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	bool bShowInitialRingNotification = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	FText InitialRingNotificationText = NSLOCTEXT("Loop9Chat", "PhoneRinging", "Phone ringing...");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring", meta = (ClampMin = "0.0"))
	float InitialRingNotificationDuration = 4.0f;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OpenChatWidget(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CloseChatWidget(APlayerController* PlayerController);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPlayerEnterRange();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPlayerExitRange();

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bPlayerInRange;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	APlayerController* CurrentPlayerController;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	UUserWidget* GetChatWidgetInstance() const { return ChatWidgetInstance; }

	/**
	 * Phantom message anomaly support: queues a fake "player" message that is
	 * injected into the chat history the next time the chat is opened.
	 */
	void QueuePhantomPlayerMessage(const FString& Message);
	void ClearPhantomPlayerMessage();

private:
	int32 ResolveCurrentLoopIndex() const;
	void RefreshLoopMessageLimitCounter();
	void LoadConfiguredOverrides();
	void EnsureStablePlayerId();
	void StopInitialRing();
	void StartInitialRingIfNeeded();
	bool ShouldStopInitialRingForLoopChange() const;
	bool ShouldUseAnomalyMumble() const;
	class UAI_ChatWidget* GetChatWidgetTyped() const;
	class UAI_ChatWidget* GetOrCreateChatWidgetTyped(APlayerController* PlayerController);
	void SetPlayerMovementEnabled(APlayerController* PlayerController, bool bEnabled);
	void ApplyInteractionInputMode(APlayerController* PlayerController, bool bUIOnly) const;

	void OnResponseReceived(FString Response);
	FString LastAIResponse;
	FString BuildSignalDropMessage() const;

	void TriggerInitialRing();
	void DispatchChatRequest(const FString& Message, bool bIsAuthRetry = false);
	bool QueuePendingAuthChat(const FString& Message, bool bIsAuthRetry, bool bForceReauth);
	void HandleLocalizedChatFailure(int32 HttpCode, int32 RetryAfterSeconds = 0);
	bool IsChatRateLimited(float* OutSecondsRemaining = nullptr) const;
	void ApplyChatRateLimitCooldown(int32 RetryAfterSeconds);
	void ClearPendingAuthChat();
	void OnAuthSessionReadyForPendingChat();
	void OnAuthSessionFailedForPendingChat(const FString& Reason);

	UPROPERTY()
	UUserWidget* ChatWidgetInstance;

	FTimerHandle InitialRingTimerHandle;
	FTimerHandle PendingAuthTimeoutHandle;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> ActiveInitialRingAudioComponent = nullptr;

	int32 RingsPlayedCount = 0;
	bool bInitialMessageInjected = false;
	int32 InitialLoopNumberAtBeginPlay = 1;
	int32 LastLoopIndexForMessageLimit = INDEX_NONE;
	int32 MessagesSentThisLoop = 0;
	double ChatRateLimitedUntilWorldTime = 0.0;
	FString PendingPhantomMessage;
	bool bPhantomMessageShown = false;
	FString PendingAuthChatMessage;
	bool bPendingAuthChat = false;
	bool bPendingAuthChatIsRetry = false;
	bool bInteractionInputCaptured = false;
	bool bSavedMovementModeValid = false;
	TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
	uint8 SavedCustomMovementMode = 0;
	uint64 ChatRequestGeneration = 0;
	FDelegateHandle AuthReadyHandle;
	FDelegateHandle AuthFailedHandle;

	UFUNCTION()
   void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
 void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
