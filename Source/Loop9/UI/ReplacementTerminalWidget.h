#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TypewriterHelper.h"
#include "ReplacementTerminalWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTerminalContinueRequested);

UCLASS()
class LOOP9_API UReplacementTerminalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void StartTerminalSequence();

	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void SimulateTypedText(const FString& InText, float InDuration, float InTypoProbability);

	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void RequestContinue();

	UPROPERTY(BlueprintAssignable, Category = "Terminal")
	FOnTerminalContinueRequested OnContinueRequested;

	UPROPERTY(BlueprintReadOnly, Category = "Terminal")
	FText TerminalText;

	UPROPERTY(BlueprintReadOnly, Category = "Terminal")
	bool bSequenceFinished = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal")
	void OnTerminalSequenceFinished();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	float DefaultLineDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	float LineGapDelay = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	float DefaultTypoProbability = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	bool bUseBlinkingCursor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	float CursorBlinkInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	FString CursorSymbol = TEXT("_");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	class USoundBase* TypingSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	FText ContinueButtonLabel = FText::FromString(TEXT("Return to Main Menu"));

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BT_Continue;

	UFUNCTION()
	void HandleContinueClicked();

private:
	void StartNextLineFromQueue();
	void TypeNextCharacter();
	void ShowContinuePrompt();
	void ToggleCursorBlink();
	void UpdateTerminalDisplay(const FString& BaseText);
	void BindContinueButton();

	TArray<FString> Lines;
	int32 CurrentLineIndex = 0;

	FString CompletedText;
	FTypewriterState ActiveTypingState;

	FTimerHandle TypingTimerHandle;
	FTimerHandle NextLineTimerHandle;
	FTimerHandle CursorBlinkTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UButton> FallbackContinueButton;

	bool bCursorVisible = true;
	FString CurrentBaseText;
};
