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

	const TArray<FText> LocalizedLines = {
		NSLOCTEXT("Loop9Terminal", "Line01", "Initializing interface..."),
		NSLOCTEXT("Loop9Terminal", "Line02", "Loading conversation model..."),
		NSLOCTEXT("Loop9Terminal", "Line03", "Importing behavioral profile..."),
		NSLOCTEXT("Loop9Terminal", "Line04", "Importing subject language patterns..."),
		NSLOCTEXT("Loop9Terminal", "Line05", "Loading response profile..."),
		NSLOCTEXT("Loop9Terminal", "Line06", "Personality imprint complete."),
		NSLOCTEXT("Loop9Terminal", "Line07", "Source: Player"),
		NSLOCTEXT("Loop9Terminal", "Line08", "Synchronization complete."),
		NSLOCTEXT("Loop9Terminal", "Line09", "User connected."),
		NSLOCTEXT("Loop9Terminal", "Line10", "User: Hello?"),
		NSLOCTEXT("Loop9Terminal", "Line11", "User: Is someone there?"),
		NSLOCTEXT("Loop9Terminal", "Line12", "User: I think I'm stuck."),
		NSLOCTEXT("Loop9Terminal", "Line13", "User: Can you help me?")
	};

	Lines.Reset(LocalizedLines.Num());
	for (const FText& Line : LocalizedLines)
	{
		Lines.Add(Line.ToString());
	}

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
