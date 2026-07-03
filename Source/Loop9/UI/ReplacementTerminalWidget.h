#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TypewriterHelper.h"
#include "ReplacementTerminalWidget.generated.h"

UCLASS()
class LOOP9_API UReplacementTerminalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void StartTerminalSequence();

	// Generic helper: type a line over time with optional typo probability (0..1)
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void SimulateTypedText(const FString& InText, float InDuration, float InTypoProbability);

	UPROPERTY(BlueprintReadOnly, Category = "Terminal")
	FText TerminalText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal")
	void OnTerminalSequenceFinished();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	float DefaultLineDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	float LineGapDelay = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	float DefaultTypoProbability = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	float FadeToBlackDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	bool bUseBlinkingCursor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	float CursorBlinkInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	FString CursorSymbol = TEXT("_");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	class USoundBase* TypingSound = nullptr;

protected:
	virtual void NativeConstruct() override;

private:
	void StartNextLineFromQueue();
	void TypeNextCharacter();
	void BeginFinalFadeAndReturnToMainMenu();
	void ToggleCursorBlink();
	void UpdateTerminalDisplay(const FString& BaseText);

	TArray<FString> Lines;
	int32 CurrentLineIndex = 0;

	FString CompletedText;
	FTypewriterState ActiveTypingState;

	FTimerHandle TypingTimerHandle;
	FTimerHandle NextLineTimerHandle;
	FTimerHandle ReturnToMenuTimerHandle;
	FTimerHandle CursorBlinkTimerHandle;
	bool bCursorVisible = true;
	FString CurrentBaseText;
};
