#include "UI/ReplacementTerminalWidget.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Sound/SoundBase.h"

void UReplacementTerminalWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UReplacementTerminalWidget::StartTerminalSequence()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(NextLineTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ReturnToMenuTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CursorBlinkTimerHandle);

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
		OnTerminalSequenceFinished();
		BeginFinalFadeAndReturnToMainMenu();
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

void UReplacementTerminalWidget::BeginFinalFadeAndReturnToMainMenu()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}

	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeToBlackDuration, FLinearColor::Black, false, true);
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CursorBlinkTimerHandle);
	}

	GetWorld()->GetTimerManager().SetTimer(
		ReturnToMenuTimerHandle,
		[this]()
		{
			if (UWorld* World = GetWorld())
			{
				UGameplayStatics::OpenLevel(World, FName("MainMenu"));
			}
		},
		FadeToBlackDuration,
		false);
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
