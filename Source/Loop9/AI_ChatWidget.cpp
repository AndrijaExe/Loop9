#include "AI_ChatWidget.h"
#include "AI_Friend.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Engine/GameViewportClient.h"
#include "Steam/Loop9SteamUtils.h"

void UAI_ChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SendButton = ResolveInnerButton(SendButton1, InnerButtonName);
	CloseButton = ResolveInnerButton(CloseButton1, InnerButtonName);

	if (SendButton)
	{
		SendButton->OnClicked.RemoveDynamic(this, &UAI_ChatWidget::OnSendButtonClicked);
		SendButton->OnClicked.AddDynamic(this, &UAI_ChatWidget::OnSendButtonClicked);
	}
	else
	{
		return;
	}

    if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UAI_ChatWidget::OnCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UAI_ChatWidget::OnCloseButtonClicked);
	}

	if (!MessageInputBox)
	{
		return;
	}

	if (!ChatScrollBox)
	{
		return;
	}

	MessageInputBox->OnTextCommitted.RemoveDynamic(this, &UAI_ChatWidget::OnMessageInputCommitted);
	MessageInputBox->OnTextCommitted.AddDynamic(this, &UAI_ChatWidget::OnMessageInputCommitted);

	if (MessageInputBox)
	{
      RequestMessageInputFocus(true);
	}
}

void UAI_ChatWidget::NativeDestruct()
{
	StopAIMumble();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AITypewriterTimerHandle);
		World->GetTimerManager().ClearTimer(AICursorBlinkTimerHandle);
		World->GetTimerManager().ClearTimer(MessageInputFocusTimerHandle);
		World->GetTimerManager().ClearTimer(ThinkingLongWaitTimerHandle);
	}

	Super::NativeDestruct();
}

void UAI_ChatWidget::StartAIMumble(bool bUseAnomalyMumble)
{
	StopAIMumble();

	USoundBase* SoundToPlay = AIMumbleSound;
	float Volume = AIMumbleVolume;

	if (bUseAnomalyMumble && AIMumbleAnomalySound)
	{
		SoundToPlay = AIMumbleAnomalySound;
		Volume = AIMumbleAnomalyVolume;
	}

	if (!SoundToPlay)
	{
		return;
	}

	ActiveAIMumbleAudioComponent = UGameplayStatics::SpawnSound2D(
		this,
		SoundToPlay,
		Volume,
		1.0f,
		0.0f,
		nullptr,
		false,
		bLoopAIMumbleWhileTyping);

	if (ActiveAIMumbleAudioComponent)
	{
		ActiveAIMumbleAudioComponent->bAutoDestroy = false;
	}
}

void UAI_ChatWidget::StopAIMumble()
{
	if (!ActiveAIMumbleAudioComponent)
	{
		return;
	}

	ActiveAIMumbleAudioComponent->Stop();
	ActiveAIMumbleAudioComponent->DestroyComponent();
	ActiveAIMumbleAudioComponent = nullptr;
}

UButton* UAI_ChatWidget::ResolveInnerButton(UUserWidget* Widget, const FName& ButtonName) const
{
	if (!Widget)
	{
		return nullptr;
	}

	UWidget* Found = Widget->GetWidgetFromName(ButtonName);
	return Cast<UButton>(Found);
}

void UAI_ChatWidget::ShowThinkingIndicator()
{
	if (!ChatScrollBox || ThinkingIndicatorText)
	{
		return;
	}

	ThinkingIndicatorText = NewObject<UTextBlock>(this);
	if (!ThinkingIndicatorText)
	{
		return;
	}

	FSlateFontInfo FontInfo = ThinkingIndicatorText->GetFont();
	FontInfo.Size = 22;
	ThinkingIndicatorText->SetFont(FontInfo);
	ThinkingIndicatorText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.55f, 0.7f)));
	ThinkingIndicatorText->SetAutoWrapText(true);

	const FString Prefix = NSLOCTEXT("Loop9Chat", "FriendPrefix", "Dragojlo: ").ToString();
	const FString Thinking = NSLOCTEXT("Loop9Chat", "ThinkingIndicator", "Thinking...").ToString();
	ThinkingIndicatorText->SetText(FText::FromString(Prefix + Thinking));

	ChatScrollBox->AddChild(ThinkingIndicatorText);
	ChatScrollBox->ScrollToEnd();
	if (MessageInputBox)
	{
		MessageInputBox->SetIsEnabled(false);
	}
	if (SendButton)
	{
		SendButton->SetIsEnabled(false);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ThinkingLongWaitTimerHandle,
			this,
			&UAI_ChatWidget::ShowLongWaitThinkingStatus,
			FMath::Max(1.0f, LongWaitStatusDelay),
			false);
	}
}

void UAI_ChatWidget::ShowLongWaitThinkingStatus()
{
	if (!ThinkingIndicatorText)
	{
		return;
	}

	const FString Prefix = NSLOCTEXT("Loop9Chat", "FriendPrefix", "Dragojlo: ").ToString();
	const FString StillThinking = NSLOCTEXT("Loop9Chat", "StillThinkingIndicator", "Still thinking...").ToString();
	ThinkingIndicatorText->SetText(FText::FromString(Prefix + StillThinking));
}

void UAI_ChatWidget::HideThinkingIndicator()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ThinkingLongWaitTimerHandle);
	}

	if (ThinkingIndicatorText && ChatScrollBox)
	{
		ChatScrollBox->RemoveChild(ThinkingIndicatorText);
	}

	ThinkingIndicatorText = nullptr;
	if (MessageInputBox)
	{
		MessageInputBox->SetIsEnabled(true);
	}
	if (SendButton)
	{
		SendButton->SetIsEnabled(true);
	}
}

void UAI_ChatWidget::AddMessageToChat(const FString& Message, bool bIsFromUser, bool bUseAnomalyMumble)
{
	if (!ChatScrollBox)
	{
		return;
	}

	if (!bIsFromUser)
	{
		HideThinkingIndicator();
	}

	UTextBlock* MessageText = NewObject<UTextBlock>(this);
	if (MessageText)
	{
		FSlateFontInfo FontInfo = MessageText->GetFont();
		FontInfo.Size = 22;
		MessageText->SetFont(FontInfo);

      FLinearColor TextColor = bIsFromUser ?
			FLinearColor(0.2f, 1.0f, 0.2f) :
			FLinearColor(0.5f, 0.8f, 1.0f);
		MessageText->SetColorAndOpacity(FSlateColor(TextColor));

		// "Dragojlo" is a character name and stays untranslated by design.
		const FString PrefixOnly = bIsFromUser
			? NSLOCTEXT("Loop9Chat", "PlayerPrefix", "You: ").ToString()
			: NSLOCTEXT("Loop9Chat", "FriendPrefix", "Dragojlo: ").ToString();

		const FString PrefixedMessage = PrefixOnly + Message;
		if (bIsFromUser || !bUseTypewriterForAI)
		{
			MessageText->SetText(FText::FromString(PrefixedMessage));
		}
		else
		{
			MessageText->SetText(FText::FromString(PrefixOnly));

			StopAIMumble();

			if (GetWorld())
			{
				GetWorld()->GetTimerManager().ClearTimer(AITypewriterTimerHandle);
				GetWorld()->GetTimerManager().ClearTimer(AICursorBlinkTimerHandle);
			}

			ActiveAITypewriterText = MessageText;
			FTypewriterHelper::Begin(AITypewriterState, Message, AITypewriterDuration, AITypewriterTypoProbability, PrefixOnly);
			AICurrentBaseText = PrefixOnly;
			bAICursorVisible = true;
			StartAIMumble(bUseAnomalyMumble);

			if (GetWorld() && bUseBlinkingCursorForAI)
			{
				GetWorld()->GetTimerManager().SetTimer(AICursorBlinkTimerHandle, this, &UAI_ChatWidget::ToggleAICursorBlink, AICursorBlinkInterval, true);
			}

			TickAITypewriter();
		}

		MessageText->SetAutoWrapText(true);

		ChatScrollBox->AddChild(MessageText);
		ChatScrollBox->ScrollToEnd();
	}
}

void UAI_ChatWidget::TickAITypewriter()
{
	if (!ActiveAITypewriterText)
	{
		return;
	}

	FString Output;
	bool bPlayTypingSound = false;
	const float NextDelay = FTypewriterHelper::Step(AITypewriterState, Output, bPlayTypingSound);
	UpdateAITypewriterDisplay(Output);

	if (bPlayTypingSound && TypingSound)
	{
		UGameplayStatics::PlaySound2D(this, TypingSound);
	}

	if (ChatScrollBox)
	{
		ChatScrollBox->ScrollToEnd();
	}

	if (AITypewriterState.bFinished)
	{
		StopAIMumble();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AICursorBlinkTimerHandle);
		}
		if (ActiveAITypewriterText)
		{
			ActiveAITypewriterText->SetText(FText::FromString(Output));
		}
		ActiveAITypewriterText = nullptr;
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AITypewriterTimerHandle, this, &UAI_ChatWidget::TickAITypewriter, NextDelay, false);
	}
}

void UAI_ChatWidget::ToggleAICursorBlink()
{
	bAICursorVisible = !bAICursorVisible;
	UpdateAITypewriterDisplay(AICurrentBaseText);
}

void UAI_ChatWidget::UpdateAITypewriterDisplay(const FString& BaseText)
{
	AICurrentBaseText = BaseText;

	if (!ActiveAITypewriterText)
	{
		return;
	}

	if (bUseBlinkingCursorForAI && !AITypewriterState.bFinished)
	{
		ActiveAITypewriterText->SetText(FText::FromString(BaseText + (bAICursorVisible ? AICursorSymbol : TEXT(""))));
	}
	else
	{
		ActiveAITypewriterText->SetText(FText::FromString(BaseText));
	}
}

void UAI_ChatWidget::OnSendButtonClicked()
{
	HandleSendMessage();
}

void UAI_ChatWidget::OnMessageInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		HandleSendMessage();
	}
}

void UAI_ChatWidget::OnCloseButtonClicked()
{
	if (AIFriendRef)
	{
		APlayerController* PC = GetOwningPlayer();
		if (PC)
		{
			AIFriendRef->CloseChatWidget(PC);
		}
	}
	else
	{
		RemoveFromParent();
	}
}

void UAI_ChatWidget::HandleSendMessage()
{
	if (!MessageInputBox)
	{
		return;
	}

	FText MessageText = MessageInputBox->GetText();
	FString Message = MessageText.ToString().TrimStartAndEnd();

	if (Message.IsEmpty())
	{
		return;
	}

	SendMessageToAI(Message);

	MessageInputBox->SetText(FText::GetEmpty());

   RequestMessageInputFocus(true);
}

void UAI_ChatWidget::RequestMessageInputFocus(bool bDelayOneTick)
{
	if (!MessageInputBox)
	{
		return;
	}

	auto FocusNow = [this]()
	{
		if (!MessageInputBox)
		{
			return;
		}

		MessageInputBox->SetUserFocus(GetOwningPlayer());
		MessageInputBox->SetKeyboardFocus();

		if (TSharedPtr<SWidget> Cached = MessageInputBox->GetCachedWidget())
		{
			FSlateApplication::Get().SetKeyboardFocus(Cached, EFocusCause::SetDirectly);
		}

		ShowOnScreenKeyboardIfNeeded();
	};

	if (!bDelayOneTick)
	{
		FocusNow();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MessageInputFocusTimerHandle);
		FTimerDelegate FocusDelegate;
		FocusDelegate.BindLambda(FocusNow);
		World->GetTimerManager().SetTimer(MessageInputFocusTimerHandle, FocusDelegate, 0.01f, false);
	}
}

void UAI_ChatWidget::ShowOnScreenKeyboardIfNeeded()
{
	if (!MessageInputBox || !FLoop9SteamUtils::IsRunningOnSteamDeck())
	{
		return;
	}

	// Tell Steam where the input field is so the floating keyboard avoids it.
	// Geometry can still be zero right after construction; fall back to the
	// bottom third of the viewport in that case.
	FVector2D FieldPos = FVector2D::ZeroVector;
	FVector2D FieldSize = FVector2D::ZeroVector;

	const FGeometry& Geometry = MessageInputBox->GetCachedGeometry();
	if (Geometry.GetLocalSize().SizeSquared() > 0.0f)
	{
		FieldPos = Geometry.GetAbsolutePosition();
		FieldSize = Geometry.GetAbsoluteSize();
	}
	else if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize = FVector2D::ZeroVector;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		FieldPos = FVector2D(0.0f, ViewportSize.Y * 0.66f);
		FieldSize = FVector2D(ViewportSize.X, ViewportSize.Y * 0.08f);
	}

	FLoop9SteamUtils::ShowOnScreenKeyboard(FieldPos, FieldSize);
}

void UAI_ChatWidget::ClearChat()
{
	if (!ChatScrollBox)
	{
		return;
	}

	StopAIMumble();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AITypewriterTimerHandle);
		World->GetTimerManager().ClearTimer(AICursorBlinkTimerHandle);
	}
	ActiveAITypewriterText = nullptr;
	HideThinkingIndicator();

	ChatScrollBox->ClearChildren();
}

void UAI_ChatWidget::SendMessageToAI(const FString& Message)
{
	if (!AIFriendRef)
	{
     TArray<AActor*> FoundAIFriends;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAI_Friend::StaticClass(), FoundAIFriends);
		if (FoundAIFriends.Num() > 0)
		{
			AIFriendRef = Cast<AAI_Friend>(FoundAIFriends[0]);
		}

		if (!AIFriendRef)
		{
			UE_LOG(LogTemp, Error, TEXT("AI_ChatWidget: AIFriendRef is null, cannot send message."));
			return;
		}
	}

	if (Message.IsEmpty())
	{
		return;
	}

	AddMessageToChat(Message, true);

	UE_LOG(LogTemp, Log, TEXT("AI_ChatWidget: Sending message to AI_Friend. Len=%d"), Message.Len());

	AIFriendRef->SayToAI(Message);
}
