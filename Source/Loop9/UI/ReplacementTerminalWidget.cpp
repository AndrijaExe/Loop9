#include "UI/ReplacementTerminalWidget.h"
#include "UI/Loop9WidgetClickBinder.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void AlignReplacementContinueButton(UWidget* ContinueButton)
	{
		FLoop9WidgetClickBinder::AlignToCanvasBottomRight(
			ContinueButton,
			FVector2D(64.0f, 80.0f),
			FVector2D(520.0f, 72.0f));
	}
}

UReplacementTerminalWidget::UReplacementTerminalWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<USoundBase> KeyFinder(
		TEXT("/Game/MyStuff/Sound/UI/TypewriterKey"));
	if (KeyFinder.Succeeded())
	{
		TypingSound = KeyFinder.Object;
	}
}

void UReplacementTerminalWidget::NativeConstruct()
{
	Super::NativeConstruct();
	StopAllAnimations();
	SetRenderOpacity(1.0f);
	BindContinueButton();
	AlignReplacementContinueButton(BT_Continue);
}

void UReplacementTerminalWidget::NativeDestruct()
{
	FLoop9WidgetClickBinder::UnbindClicked(
		BT_Continue, this, GET_FUNCTION_NAME_CHECKED(UReplacementTerminalWidget, HandleContinueClicked));

	if (FallbackContinueButton)
	{
		FallbackContinueButton->OnClicked.RemoveDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TypingTimerHandle);
		World->GetTimerManager().ClearTimer(NextLineTimerHandle);
		World->GetTimerManager().ClearTimer(CursorBlinkTimerHandle);
		World->GetTimerManager().ClearTimer(YouPromptHoldTimerHandle);
	}

	Super::NativeDestruct();
}

void UReplacementTerminalWidget::RequestContinue()
{
	OnContinueRequested.Broadcast();
}

void UReplacementTerminalWidget::HandleContinueClicked()
{
	if (IsRevealing())
	{
		FinishReveal();
		return;
	}

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
	GetWorld()->GetTimerManager().ClearTimer(YouPromptHoldTimerHandle);

	bSequenceFinished = false;
	bHoldingYouPrompt = false;

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
		NSLOCTEXT("Loop9Terminal", "Line11", "User: Is someone there?")
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

	SetContinueVisible(true);
	RefreshContinueButtonLabel();

	if (bUseBlinkingCursor)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CursorBlinkTimerHandle,
			this,
			&UReplacementTerminalWidget::ToggleCursorBlink,
			CursorBlinkInterval,
			true);
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
		BeginYouPromptHold();
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
		UGameplayStatics::PlaySound2D(
			this,
			TypingSound,
			TypingSoundVolume,
			FMath::FRandRange(0.92f, 1.08f));
	}

	if (ActiveTypingState.bFinished)
	{
		if (!CompletedText.IsEmpty())
		{
			CompletedText += TEXT("\n");
		}
		CompletedText += CurrentTypedOutput;
		UpdateTerminalDisplay(CompletedText);

		GetWorld()->GetTimerManager().SetTimer(
			NextLineTimerHandle,
			this,
			&UReplacementTerminalWidget::StartNextLineFromQueue,
			LineGapDelay,
			false);
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		TypingTimerHandle,
		this,
		&UReplacementTerminalWidget::TypeNextCharacter,
		NextDelay,
		false);
}

void UReplacementTerminalWidget::BeginYouPromptHold()
{
	bSequenceFinished = true;
	bHoldingYouPrompt = true;
	OnTerminalSequenceFinished();
	RefreshContinueButtonLabel();

	if (!CompletedText.IsEmpty())
	{
		CompletedText += TEXT("\n");
	}
	CompletedText += TEXT("You:");
	UpdateTerminalDisplay(CompletedText);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			YouPromptHoldTimerHandle,
			this,
			&UReplacementTerminalWidget::FinishYouPromptHold,
			FMath::Max(0.5f, YouPromptHoldSeconds),
			false);
	}
}

void UReplacementTerminalWidget::FinishYouPromptHold()
{
	bHoldingYouPrompt = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CursorBlinkTimerHandle);
	}

	UpdateTerminalDisplay(CompletedText);

	if (PromptCompleteSound)
	{
		UGameplayStatics::PlaySound2D(this, PromptCompleteSound);
	}

	RefreshContinueButtonLabel();
}

void UReplacementTerminalWidget::FinishReveal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TypingTimerHandle);
		World->GetTimerManager().ClearTimer(NextLineTimerHandle);
		World->GetTimerManager().ClearTimer(YouPromptHoldTimerHandle);
		World->GetTimerManager().ClearTimer(CursorBlinkTimerHandle);
	}

	CompletedText.Empty();
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		if (Index > 0)
		{
			CompletedText += TEXT("\n");
		}
		CompletedText += Lines[Index];
	}
	if (!CompletedText.IsEmpty())
	{
		CompletedText += TEXT("\n");
	}
	CompletedText += TEXT("You:");

	CurrentLineIndex = Lines.Num();
	ActiveTypingState.bFinished = true;
	bSequenceFinished = true;
	bHoldingYouPrompt = false;
	bCursorVisible = false;
	UpdateTerminalDisplay(CompletedText);

	if (PromptCompleteSound)
	{
		UGameplayStatics::PlaySound2D(this, PromptCompleteSound);
	}

	RefreshContinueButtonLabel();
}

void UReplacementTerminalWidget::BindContinueButton()
{
	if (BT_Continue)
	{
		FLoop9WidgetClickBinder::UnbindClicked(
			BT_Continue, this, GET_FUNCTION_NAME_CHECKED(UReplacementTerminalWidget, HandleContinueClicked));
		FLoop9WidgetClickBinder::BindClicked(
			BT_Continue, this, GET_FUNCTION_NAME_CHECKED(UReplacementTerminalWidget, HandleContinueClicked));
		BT_Continue->SetVisibility(ESlateVisibility::Visible);
		AlignReplacementContinueButton(BT_Continue);
	}

	if (FallbackContinueButton)
	{
		FallbackContinueButton->OnClicked.RemoveDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);
		FallbackContinueButton->OnClicked.AddDynamic(this, &UReplacementTerminalWidget::HandleContinueClicked);
		FallbackContinueButton->SetVisibility(ESlateVisibility::Visible);
	}

	RefreshContinueButtonLabel();
}

void UReplacementTerminalWidget::RefreshContinueButtonLabel()
{
	const FText Label = IsRevealing() ? SkipTypingButtonLabel : ContinueButtonLabel;

	if (BT_Continue)
	{
		FLoop9WidgetClickBinder::SetButtonText(BT_Continue, Label);
	}

	if (FallbackContinueButton)
	{
		FLoop9WidgetClickBinder::SetButtonText(FallbackContinueButton, Label);
	}
}

void UReplacementTerminalWidget::SetContinueVisible(bool bVisible)
{
	const ESlateVisibility ContinueVisibility = bVisible
		? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed;

	if (BT_Continue)
	{
		BT_Continue->SetVisibility(ContinueVisibility);
	}
	if (FallbackContinueButton)
	{
		FallbackContinueButton->SetVisibility(ContinueVisibility);
	}
}

bool UReplacementTerminalWidget::IsRevealing() const
{
	return !bSequenceFinished || bHoldingYouPrompt;
}

void UReplacementTerminalWidget::ToggleCursorBlink()
{
	bCursorVisible = !bCursorVisible;
	UpdateTerminalDisplay(CurrentBaseText);
}

void UReplacementTerminalWidget::UpdateTerminalDisplay(const FString& BaseText)
{
	CurrentBaseText = BaseText;

	const bool bShowCursor = bUseBlinkingCursor
		&& (bHoldingYouPrompt || !ActiveTypingState.bFinished || !bSequenceFinished);

	if (bShowCursor)
	{
		TerminalText = FText::FromString(BaseText + (bCursorVisible ? CursorSymbol : TEXT("")));
	}
	else
	{
		TerminalText = FText::FromString(BaseText);
	}
}
