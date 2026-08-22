#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Loop/LoopTypes.h"
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
};
