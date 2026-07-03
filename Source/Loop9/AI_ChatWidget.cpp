#include "AI_ChatWidget.h"
#include "AI_Friend.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

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

UButton* UAI_ChatWidget::ResolveInnerButton(UUserWidget* Widget, const FName& ButtonName) const
{
	if (!Widget)
	{
		return nullptr;
	}

	UWidget* Found = Widget->GetWidgetFromName(ButtonName);
	return Cast<UButton>(Found);
}

void UAI_ChatWidget::AddMessageToChat(const FString& Message, bool bIsFromUser)
{
	if (!ChatScrollBox)
	{
		return;
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

       FString PrefixedMessage = bIsFromUser ?
			FString::Printf(TEXT("You: %s"), *Message) :
			FString::Printf(TEXT("Dragojlo: %s"), *Message);

		const FString PrefixOnly = bIsFromUser ? TEXT("You: ") : TEXT("Dragojlo: ");
		if (bIsFromUser || !bUseTypewriterForAI)
		{
			MessageText->SetText(FText::FromString(PrefixedMessage));
		}
		else
		{
			MessageText->SetText(FText::FromString(PrefixOnly));

			if (GetWorld())
			{
				GetWorld()->GetTimerManager().ClearTimer(AITypewriterTimerHandle);
				GetWorld()->GetTimerManager().ClearTimer(AICursorBlinkTimerHandle);
			}

			ActiveAITypewriterText = MessageText;
			FTypewriterHelper::Begin(AITypewriterState, Message, AITypewriterDuration, AITypewriterTypoProbability, PrefixOnly);
			AICurrentBaseText = PrefixOnly;
			bAICursorVisible = true;

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

void UAI_ChatWidget::ClearChat()
{
	if (!ChatScrollBox)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AITypewriterTimerHandle);
		World->GetTimerManager().ClearTimer(AICursorBlinkTimerHandle);
	}
	ActiveAITypewriterText = nullptr;

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

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GI->GetSubsystem<ULoopManagerSubsystem>())
		{
			LoopManager->RegisterAIInteraction();
		}
	}
}
