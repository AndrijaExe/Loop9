#include "UI/EndingWidget.h"

#include "UI/Loop9WidgetClickBinder.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "Loop9Endings"

void UEndingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayoutIfNeeded();
	BindContinueButton();
	RefreshBoundWidgets();
}

void UEndingWidget::NativeDestruct()
{
	if (FallbackContinueButton)
	{
		FallbackContinueButton->OnClicked.RemoveDynamic(this, &UEndingWidget::HandleContinueClicked);
	}

	FLoop9WidgetClickBinder::UnbindClicked(
		BT_Continue, this, GET_FUNCTION_NAME_CHECKED(UEndingWidget, HandleContinueClicked));

	Super::NativeDestruct();
}

void UEndingWidget::InitializeEnding(ELoopEndingType EndingType, int32 InResets, int32 InAIInteractions)
{
	CurrentEndingType = EndingType;
	ContinueButtonLabel = DefaultContinueButtonLabel;

	switch (EndingType)
	{
	case ELoopEndingType::EscapeTogether:
		EndingTitle = LOCTEXT("EscapeTogetherTitle", "ESCAPE TOGETHER");
		EndingDescription = LOCTEXT("EscapeTogetherDesc", "You and Dragojlo built trust without losing yourself. You escaped the loop together.");
		break;
	case ELoopEndingType::ObedientFool:
		EndingTitle = LOCTEXT("ObedientFoolTitle", "OBEDIENT FOOL");
		EndingDescription = LOCTEXT("ObedientFoolDesc", "You trusted completely and stopped deciding for yourself. The wrong voice chose your fate.");
		break;
	case ELoopEndingType::ColdBetrayal:
		EndingTitle = LOCTEXT("ColdBetrayalTitle", "COLD BETRAYAL");
		EndingDescription = LOCTEXT("ColdBetrayalDesc", "You relied on him but never respected him. One calm lie was enough.");
		break;
	case ELoopEndingType::MergedMemory:
		EndingTitle = LOCTEXT("MergedMemoryTitle", "MERGED MEMORY");
		EndingDescription = LOCTEXT("MergedMemoryDesc", "Your memories and his leaks intertwined. You can no longer tell who remembers what.");
		break;
	case ELoopEndingType::TheReplacement:
		EndingTitle = LOCTEXT("TheReplacementTitle", "THE REPLACEMENT");
		EndingDescription = LOCTEXT("TheReplacementDesc", "He did not destroy you. He learned you. Now you are the voice on the other side of the line.");
		break;
	default:
		EndingTitle = LOCTEXT("ParanoidSurvivorTitle", "PARANOID SURVIVOR");
		EndingDescription = LOCTEXT("ParanoidSurvivorDesc", "You escaped alone by trusting nobody. Maybe that saved you. Maybe it cost you the truth.");
		break;
	}

	EndingStats = FText::Format(
		LOCTEXT("EndingStatsFormat", "Resets: {0} | AI interactions: {1}"),
		FText::AsNumber(InResets), FText::AsNumber(InAIInteractions));

	RefreshBoundWidgets();
	BP_OnEndingInitialized(EndingType);
}

void UEndingWidget::RequestContinue()
{
	OnContinueRequested.Broadcast();
}

void UEndingWidget::HandleContinueClicked()
{
	RequestContinue();
}

void UEndingWidget::RefreshBoundWidgets()
{
	if (TB_Title)
	{
		TB_Title->SetText(EndingTitle);
	}

	if (TB_Description)
	{
		TB_Description->SetText(EndingDescription);
		// Do not force WrapTextAt / slot sizes — that was pushing the blurb off-screen
		// in the authored WBP layout. Keep Blueprint anchors/position intact.
		TB_Description->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TB_Description->SetRenderOpacity(1.0f);
	}

	if (TB_Stats)
	{
		TB_Stats->SetText(EndingStats);
	}
}

void UEndingWidget::BuildFallbackLayoutIfNeeded()
{
	if (TB_Title || TB_Description || !WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	if (UCanvasPanelSlot* VBoxSlot = Cast<UCanvasPanelSlot>(Root->AddChild(VBox)))
	{
		VBoxSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		VBoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		VBoxSlot->SetAutoSize(true);
	}

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Bold", 48)));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TitleText->SetJustification(ETextJustify::Center);
	VBox->AddChildToVerticalBox(TitleText);
	TB_Title = TitleText;

	UTextBlock* DescText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescText"));
	DescText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Regular", 24)));
	DescText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f)));
	DescText->SetJustification(ETextJustify::Center);
	DescText->SetAutoWrapText(true);
	DescText->SetWrapTextAt(780.0f);
	VBox->AddChildToVerticalBox(DescText);
	TB_Description = DescText;

	UTextBlock* StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatsText"));
	StatsText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Regular", 18)));
	StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f, 1.0f)));
	StatsText->SetJustification(ETextJustify::Center);
	VBox->AddChildToVerticalBox(StatsText);
	TB_Stats = StatsText;

	FallbackContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContinueButton"));
	UTextBlock* ContinueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContinueLabel"));
	ContinueLabel->SetText(ContinueButtonLabel);
	ContinueLabel->SetJustification(ETextJustify::Center);
	FallbackContinueButton->AddChild(ContinueLabel);
	VBox->AddChildToVerticalBox(FallbackContinueButton);
}

void UEndingWidget::BindContinueButton()
{
	if (BT_Continue)
	{
		FLoop9WidgetClickBinder::UnbindClicked(
			BT_Continue, this, GET_FUNCTION_NAME_CHECKED(UEndingWidget, HandleContinueClicked));
		FLoop9WidgetClickBinder::BindClicked(
			BT_Continue, this, GET_FUNCTION_NAME_CHECKED(UEndingWidget, HandleContinueClicked));
		FLoop9WidgetClickBinder::SetButtonText(BT_Continue, ContinueButtonLabel);
		BT_Continue->SetVisibility(ESlateVisibility::Visible);
	}

	if (FallbackContinueButton)
	{
		FallbackContinueButton->OnClicked.RemoveDynamic(this, &UEndingWidget::HandleContinueClicked);
		FallbackContinueButton->OnClicked.AddDynamic(this, &UEndingWidget::HandleContinueClicked);
		FallbackContinueButton->SetVisibility(ESlateVisibility::Visible);
	}
}

#undef LOCTEXT_NAMESPACE
