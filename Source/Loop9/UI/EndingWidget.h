#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Loop/LoopTypes.h"
#include "UI/TypewriterHelper.h"
#include "EndingWidget.generated.h"

class UButton;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndingContinueRequested);

UCLASS()
class LOOP9_API UEndingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UEndingWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Ending")
	void InitializeEnding(ELoopEndingType EndingType, int32 InResets, int32 InAIInteractions);

	UFUNCTION(BlueprintCallable, Category = "Ending")
	void RequestContinue();

	UFUNCTION(BlueprintImplementableEvent, Category = "Ending", meta = (DisplayName = "On Ending Initialized"))
	void BP_OnEndingInitialized(ELoopEndingType EndingType);

	UPROPERTY(BlueprintAssignable, Category = "Ending")
	FOnEndingContinueRequested OnContinueRequested;

	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	ELoopEndingType CurrentEndingType = ELoopEndingType::ObedientFool;

	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	FText EndingTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	FText EndingDescription;

	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	FText EndingStats;

	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	FText ContinueButtonLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	FText DefaultContinueButtonLabel = NSLOCTEXT("Loop9Endings", "ContinueButtonLabel", "Return to Main Menu");

	/** Type the closing line out a character at a time instead of pasting it up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Typing")
	bool bTypeOutDescription = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Typing", meta = (ClampMin = "1.0"))
	float TypingCharactersPerSecond = 20.0f;

	/** One keypress. Pitch is randomised per character so a line is not a single note. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Typing")
	TObjectPtr<class USoundBase> TypingSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Typing", meta = (ClampMin = "0.0"))
	float TypingSoundVolume = 0.55f;

	/** Chance per letter that he mistypes and backs up over it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Typing", meta = (ClampMin = "0.0", ClampMax = "0.2"))
	float TypingTypoProbability = 0.02f;

	/** Shown on the continue button while the line is still typing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Typing")
	FText SkipTypingButtonLabel = NSLOCTEXT("Loop9Endings", "SkipTypingLabel", "SKIP");

	/** Reveal the rest of the line immediately. */
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void FinishTyping();

	UFUNCTION(BlueprintPure, Category = "Ending")
	bool IsTyping() const { return bIsTyping; }

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Title;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Stats;

	/** Raw UButton or WBP_Button instance named BT_Continue. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BT_Continue;

	/** Optional authored host. C++ fills it from BuildRunEventCards() if present. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_Timeline;

	UFUNCTION()
	void HandleContinueClicked();

private:
	void RefreshBoundWidgets();
	void BuildFallbackLayoutIfNeeded();
	void BindContinueButton();
	void EnsureTimelineHost();
	void StartTyping();
	void StopTypingTimer();
	void TypeNextCharacter();
	void ApplyDescriptionText();
	void RefreshContinueButtonLabel();
	void PopulateTimeline();
	void AddTimelineRow(const FRunEventCard& Card);
	UTexture2D* TimelineIconFor(ERunEventType Type) const;

	UPROPERTY(Transient)
	TObjectPtr<UButton> FallbackContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> TimelineScroll;

	UPROPERTY()
	TObjectPtr<UTexture2D> CallIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> LiftIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> EndingIcon;

	FString FullDescription;
	FString TypedDescription;
	FTypewriterState TypingState;
	bool bIsTyping = false;
	FTimerHandle TypingTimerHandle;
};
