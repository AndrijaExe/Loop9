#include "UI/ReplacementTerminalWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Sound/SoundBase.h"

void UReplacementTerminalWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindContinueButton();
}

void UReplacementTerminalWidget::NativeDestruct()
{
	if (BT_Continue)
	{
		BT_Continue->OnClicked.RemoveDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);
	}

	if (FallbackContinueButton)
	{
		FallbackContinueButton->OnClicked.RemoveDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);
	}

	Super::NativeDestruct();
}

void UReplacementTerminalWidget::RequestContinue()
{
	OnContinueRequested.Broadcast();
}

void UReplacementTerminalWidget::HandleContinueClicked()
{
	RequestContinue();
}

void UReplacementTerminalWidget::StartTerminalSequence()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(NextLineTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CursorBlinkTimerHandle);

	bSequenceFinished = false;

	Lines = {
		TEXT("Initializing interface..."),
		TEXT("Loading conversation model..."),
		TEXT("Importing behavioral profile..."),
		TEXT("Importing subject language patterns..."),
		TEXT("Loading response profile..."),
		TEXT("Personality imprint complete."),
		TEXT("Source: Player"),
		TEXT("Synchronization complete."),
		TEXT("User connected."),
		TEXT("User: Hello?"),
		TEXT("User: Is someone there?"),
		TEXT("User: I think I'm stuck."),
		TEXT("User: Can you help me?")
	};

	CurrentLineIndex = 0;
	CompletedText.Empty();
	ActiveTypingState = FTypewriterState();
	TerminalText = FText::GetEmpty();
	bCursorVisible = true;
	CurrentBaseText.Empty();

	if (bUseBlinkingCursor)
	{
		GetWorld()->GetTimerManager().SetTimer(CursorBlinkTimerHandle, this, &UReplacementTerminalWidget::ToggleCursorBlink, CursorBlinkInterval, true);
	}

	StartNextLineFromQueue();
}

void UReplacementTerminalWidget::SimulateTypedText(const FString& InText, float InDuration, float InTypoProbability)
{
	if (!GetWorld())
	{
		return;
	}

	FTypewriterHelper::Begin(ActiveTypingState, InText, InDuration, InTypoProbability);

	TypeNextCharacter();
}

void UReplacementTerminalWidget::StartNextLineFromQueue()
{
	if (!GetWorld())
	{
		return;
	}

	if (!Lines.IsValidIndex(CurrentLineIndex))
	{
		bSequenceFinished = true;
		OnTerminalSequenceFinished();
		ShowContinuePrompt();
		return;
	}

	SimulateTypedText(Lines[CurrentLineIndex], DefaultLineDuration, DefaultTypoProbability);
	CurrentLineIndex++;
}

void UReplacementTerminalWidget::TypeNextCharacter()
{
	if (!GetWorld())
	{
		return;
	}

	FString CurrentTypedOutput;
	bool bPlayTypingSound = false;
	const float NextDelay = FTypewriterHelper::Step(ActiveTypingState, CurrentTypedOutput, bPlayTypingSound);

	FString PreviewBase = CompletedText;
	if (!PreviewBase.IsEmpty())
	{
		PreviewBase += TEXT("\n");
	}
	UpdateTerminalDisplay(PreviewBase + CurrentTypedOutput);

	if (bPlayTypingSound && TypingSound)
	{
		UGameplayStatics::PlaySound2D(this, TypingSound);
	}

	if (ActiveTypingState.bFinished)
	{
		if (!CompletedText.IsEmpty())
		{
			CompletedText += TEXT("\n");
		}
		CompletedText += CurrentTypedOutput;
		UpdateTerminalDisplay(CompletedText);

		GetWorld()->GetTimerManager().SetTimer(NextLineTimerHandle, this, &UReplacementTerminalWidget::StartNextLineFromQueue, LineGapDelay, false);
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(TypingTimerHandle, this, &UReplacementTerminalWidget::TypeNextCharacter, NextDelay, false);
}

void UReplacementTerminalWidget::ShowContinuePrompt()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CursorBlinkTimerHandle);
	}

	UpdateTerminalDisplay(CompletedText);

	if (BT_Continue)
	{
		BT_Continue->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	if (FallbackContinueButton)
	{
		FallbackContinueButton->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	FallbackContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TerminalContinueButton"));
	if (!FallbackContinueButton)
	{
		return;
	}

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TerminalContinueLabel"));
	Label->SetText(ContinueButtonLabel);
	Label->SetJustification(ETextJustify::Center);
	FallbackContinueButton->AddChild(Label);
	FallbackContinueButton->OnClicked.AddDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);

	if (UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget))
	{
		RootPanel->AddChild(FallbackContinueButton);
	}

	FallbackContinueButton->SetVisibility(ESlateVisibility::Visible);
}

void UReplacementTerminalWidget::BindContinueButton()
{
	if (BT_Continue)
	{
		BT_Continue->OnClicked.RemoveDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);
		BT_Continue->OnClicked.AddDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);
		BT_Continue->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (FallbackContinueButton)
	{
		FallbackContinueButton->OnClicked.RemoveDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);
		FallbackContinueButton->OnClicked.AddDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);
		FallbackContinueButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UReplacementTerminalWidget::ToggleCursorBlink()
{
	bCursorVisible = !bCursorVisible;
	UpdateTerminalDisplay(CurrentBaseText);
}

void UReplacementTerminalWidget::UpdateTerminalDisplay(const FString& BaseText)
{
	CurrentBaseText = BaseText;

	if (bUseBlinkingCursor && !ActiveTypingState.bFinished)
	{
		TerminalText = FText::FromString(BaseText + (bCursorVisible ? CursorSymbol : TEXT("")));
	}
	else
	{
		TerminalText = FText::FromString(BaseText);
	}
}
