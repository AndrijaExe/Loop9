#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Sound/SoundBase.h"
#include "Types/SlateEnums.h"
#include "UI/TypewriterHelper.h"

#include "AI_ChatWidget.generated.h"

UCLASS()
class LOOP9_API UAI_ChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UAI_ChatWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "AI Chat")
	void SendMessageToAI(const FString& Message);

	UPROPERTY(BlueprintReadWrite, Category = "AI Chat")
	class AAI_Friend* AIFriendRef;

	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* MessageInputBox;

  UPROPERTY(meta = (BindWidget))
	UUserWidget* SendButton1 = nullptr;

	UPROPERTY(meta = (BindWidget))
	class UScrollBox* ChatScrollBox;

  UPROPERTY(meta = (BindWidget))
	UUserWidget* CloseButton1 = nullptr;

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** OverridePrefix replaces the default "You: " / "Dragojlo: " speaker label, e.g. "Someone: " for the ringing-phone caller. */
	void AddMessageToChat(const FString& Message, bool bIsFromUser, bool bUseAnomalyMumble = false, const FText& OverridePrefix = FText::GetEmpty());
	void ShowThinkingIndicator();
	void HideThinkingIndicator();

	/** 1.1: read-only chat — a send attempt plays PlayUnavailableSound() instead of sending. Cleared by the next unlock. */
	void SetInputLocked(bool bLocked);
	bool IsInputLocked() const { return bInputLocked; }

	/** "No signal" stinger for a blocked send: locked chat, or Dragojlo's per-loop message cap. Caps itself at UnavailableSoundMaxSeconds and stops on close. */
	UFUNCTION(BlueprintCallable, Category = "AI Chat")
	void PlayUnavailableSound();
	void StopUnavailableSound();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Voice")
	class USoundBase* UnavailableSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Voice", meta = (ClampMin = "0.1"))
	float UnavailableSoundMaxSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Typing")
	bool bUseTypewriterForAI = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Typing")
	float AITypewriterDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Typing")
	float AITypewriterTypoProbability = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Typing")
	bool bUseBlinkingCursorForAI = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Typing")
	float AICursorBlinkInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Typing")
	FString AICursorSymbol = TEXT("_");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Status", meta = (ClampMin = "1.0"))
	float LongWaitStatusDelay = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Typing")
	class USoundBase* TypingSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Voice", meta = (DisplayName = "AI Mumble Sound (Normal)"))
	class USoundBase* AIMumbleSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Voice", meta = (DisplayName = "AI Mumble Sound (Anomaly / Crazy)"))
	class USoundBase* AIMumbleAnomalySound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Voice", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AIMumbleVolume = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Voice", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AIMumbleAnomalyVolume = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Chat|Voice")
	bool bLoopAIMumbleWhileTyping = true;

	UFUNCTION(BlueprintCallable, Category = "AI Chat")
	void ClearChat();

	bool bInputLocked = false;

private:
	UFUNCTION()
	void OnSendButtonClicked();

	UFUNCTION()
	void OnCloseButtonClicked();

	void HandleSendMessage();

	UFUNCTION()
	void OnMessageInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void RequestMessageInputFocus(bool bDelayOneTick);
	void ShowOnScreenKeyboardIfNeeded();
	UButton* ResolveInnerButton(UUserWidget* Widget, const FName& ButtonName) const;

	void TickAITypewriter();
	void ToggleAICursorBlink();
	void UpdateAITypewriterDisplay(const FString& BaseText);
	void ShowLongWaitThinkingStatus();
	void StartAIMumble(bool bUseAnomalyMumble);
	void StopAIMumble();

	FTimerHandle AITypewriterTimerHandle;
	FTimerHandle AICursorBlinkTimerHandle;
	FTimerHandle MessageInputFocusTimerHandle;
	FTimerHandle ThinkingLongWaitTimerHandle;
	UButton* SendButton = nullptr;
	UButton* CloseButton = nullptr;
	FTypewriterState AITypewriterState;
	TObjectPtr<class UTextBlock> ActiveAITypewriterText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> ThinkingIndicatorText = nullptr;
	bool bAICursorVisible = true;
	FString AICurrentBaseText;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> ActiveAIMumbleAudioComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> ActiveUnavailableAudioComponent = nullptr;

	FTimerHandle UnavailableSoundTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "AI Chat")
	FName InnerButtonName = TEXT("Button_45");
};
