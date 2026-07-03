#include "UI/EndingWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

void UEndingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || WidgetTree->RootWidget)
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
	TitleText->SetText(EndingTitle.IsEmpty() ? FText::FromString(TEXT("ENDING")) : EndingTitle);
	VBox->AddChildToVerticalBox(TitleText);

	UTextBlock* DescText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescText"));
	DescText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Regular", 24)));
	DescText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f)));
	DescText->SetJustification(ETextJustify::Center);
	DescText->SetAutoWrapText(true);
	DescText->SetText(EndingDescription);
	VBox->AddChildToVerticalBox(DescText);

	UTextBlock* StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatsText"));
	StatsText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Regular", 18)));
	StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f, 1.0f)));
	StatsText->SetJustification(ETextJustify::Center);
	StatsText->SetText(EndingStats);
	VBox->AddChildToVerticalBox(StatsText);
}

void UEndingWidget::InitializeEnding(ELoopEndingType EndingType, int32 InResets, int32 InAIInteractions)
{
	switch (EndingType)
	{
	case ELoopEndingType::EscapeTogether:
		EndingTitle = FText::FromString(TEXT("ENDING UNLOCKED: ESCAPE TOGETHER"));
		EndingDescription = FText::FromString(TEXT("You and Dragojlo built trust without losing yourself. You escaped the loop together."));
		break;
	case ELoopEndingType::ObedientFool:
		EndingTitle = FText::FromString(TEXT("ENDING UNLOCKED: OBEDIENT FOOL"));
		EndingDescription = FText::FromString(TEXT("You trusted completely and stopped deciding for yourself. The wrong voice chose your fate."));
		break;
	case ELoopEndingType::ColdBetrayal:
		EndingTitle = FText::FromString(TEXT("ENDING UNLOCKED: COLD BETRAYAL"));
		EndingDescription = FText::FromString(TEXT("You relied on him but never respected him. One calm lie was enough."));
		break;
	case ELoopEndingType::MergedMemory:
		EndingTitle = FText::FromString(TEXT("ENDING UNLOCKED: MERGED MEMORY"));
		EndingDescription = FText::FromString(TEXT("Your memories and his leaks intertwined. You can no longer tell who remembers what."));
		break;
	case ELoopEndingType::TheReplacement:
		EndingTitle = FText::FromString(TEXT("ENDING UNLOCKED: THE REPLACEMENT"));
		EndingDescription = FText::FromString(TEXT("He did not destroy you. He learned you. Now you are the voice on the other side of the line."));
		break;
	default:
		EndingTitle = FText::FromString(TEXT("ENDING UNLOCKED: PARANOID SURVIVOR"));
		EndingDescription = FText::FromString(TEXT("You escaped alone by trusting nobody. Maybe that saved you. Maybe it cost you the truth."));
		break;
	}

	EndingStats = FText::FromString(FString::Printf(TEXT("Resets: %d | AI interactions: %d"), InResets, InAIInteractions));
}
