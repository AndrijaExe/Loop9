#include "UI/EndingWidget.h"

#include "UI/Loop9WidgetClickBinder.h"
#include "Subsystems/RelationshipSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "Loop9Endings"

UEndingWidget::UEndingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> CallFinder(
		TEXT("/Game/MyStuff/UI/Timeline/ui_icon_call"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> LiftFinder(
		TEXT("/Game/MyStuff/UI/Timeline/ui_icon_lift"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> EndingFinder(
		TEXT("/Game/MyStuff/UI/Timeline/ui_icon_ending"));
	if (CallFinder.Succeeded())
	{
		CallIcon = CallFinder.Object;
	}
	if (LiftFinder.Succeeded())
	{
		LiftIcon = LiftFinder.Object;
	}
	if (EndingFinder.Succeeded())
	{
		EndingIcon = EndingFinder.Object;
	}
}

void UEndingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayoutIfNeeded();
	BindContinueButton();
	RefreshBoundWidgets();
	PopulateTimeline();
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
	PopulateTimeline();
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
		// Replaced by the session timeline. Keep the widget for layout reuse,
		// but do not show raw Resets | AI interactions.
		TB_Stats->SetVisibility(ESlateVisibility::Collapsed);
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
	TB_Stats->SetVisibility(ESlateVisibility::Collapsed);

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("TimelineScroll"));
	USizeBox* ScrollSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TimelineSize"));
	ScrollSize->SetHeightOverride(280.0f);
	ScrollSize->SetWidthOverride(780.0f);
	UVerticalBox* Timeline = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Timeline"));
	Scroll->AddChild(Timeline);
	ScrollSize->SetContent(Scroll);
	VBox->AddChildToVerticalBox(ScrollSize);
	VB_Timeline = Timeline;
	TimelineScroll = Scroll;

	FallbackContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContinueButton"));
	UTextBlock* ContinueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContinueLabel"));
	ContinueLabel->SetText(ContinueButtonLabel);
	ContinueLabel->SetJustification(ETextJustify::Center);
	FallbackContinueButton->AddChild(ContinueLabel);
	VBox->AddChildToVerticalBox(FallbackContinueButton);
}

void UEndingWidget::EnsureTimelineHost()
{
	if (VB_Timeline || !WidgetTree)
	{
		return;
	}

	if (UVerticalBox* Named = Cast<UVerticalBox>(GetWidgetFromName(TEXT("VB_Timeline"))))
	{
		VB_Timeline = Named;
		return;
	}

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("TimelineScroll"));
	UVerticalBox* Timeline = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Timeline"));
	Scroll->AddChild(Timeline);
	VB_Timeline = Timeline;
	TimelineScroll = Scroll;

	UCanvasPanel* Canvas = nullptr;
	UCanvasPanelSlot* TemplateSlot = nullptr;
	if (TB_Stats)
	{
		Canvas = Cast<UCanvasPanel>(TB_Stats->GetParent());
		TemplateSlot = Cast<UCanvasPanelSlot>(TB_Stats->Slot);
	}
	if (!Canvas && TB_Description)
	{
		Canvas = Cast<UCanvasPanel>(TB_Description->GetParent());
		TemplateSlot = Cast<UCanvasPanelSlot>(TB_Description->Slot);
	}
	if (!Canvas)
	{
		Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}

	if (Canvas)
	{
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Scroll))
		{
			if (TemplateSlot)
			{
				Slot->SetAnchors(TemplateSlot->GetAnchors());
				Slot->SetAlignment(TemplateSlot->GetAlignment());
				Slot->SetAutoSize(false);
				const FVector2D TemplateSize = TemplateSlot->GetSize();
				const float Width = FMath::Max(TemplateSize.X, 640.0f);
				FVector2D Position = TemplateSlot->GetPosition();
				if (TB_Stats && TemplateSlot == Cast<UCanvasPanelSlot>(TB_Stats->Slot))
				{
					Slot->SetPosition(Position);
				}
				else
				{
					const float OffsetY = FMath::Max(TemplateSize.Y, 72.0f) + 16.0f;
					Slot->SetPosition(Position + FVector2D(0.0f, OffsetY));
				}
				Slot->SetSize(FVector2D(Width, 280.0f));
				Slot->SetZOrder(TemplateSlot->GetZOrder());
			}
			else
			{
				Slot->SetAnchors(FAnchors(0.08f, 0.42f, 0.62f, 0.82f));
				Slot->SetOffsets(FMargin(0.0f));
			}
		}
		return;
	}

	if (UVerticalBox* ParentBox = TB_Description ? Cast<UVerticalBox>(TB_Description->GetParent()) : nullptr)
	{
		USizeBox* ScrollSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TimelineSize"));
		ScrollSize->SetHeightOverride(280.0f);
		ScrollSize->SetContent(Scroll);
		const int32 DescIndex = ParentBox->GetChildIndex(TB_Description);
		ParentBox->InsertChildAt(DescIndex + 1, ScrollSize);
	}
}

void UEndingWidget::PopulateTimeline()
{
	EnsureTimelineHost();
	if (!VB_Timeline)
	{
		return;
	}

	VB_Timeline->ClearChildren();

	TArray<FRunEventCard> Cards;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (URelationshipSubsystem* Relationship = GI->GetSubsystem<URelationshipSubsystem>())
		{
			Cards = Relationship->BuildRunEventCards();
		}
	}

	for (const FRunEventCard& Card : Cards)
	{
		AddTimelineRow(Card);
	}
}

void UEndingWidget::AddTimelineRow(const FRunEventCard& Card)
{
	if (!VB_Timeline || !WidgetTree)
	{
		return;
	}

	const FLinearColor IceBlue(0.50f, 0.80f, 1.00f, 1.00f);
	const FLinearColor CardFill(0.04f, 0.07f, 0.10f, 0.92f);
	const FLinearColor Ring = Card.RingColor;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	USizeBox* RailBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RailBox->SetWidthOverride(4.0f);
	RailBox->SetHeightOverride(56.0f);
	UBorder* Rail = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Rail->SetBrushColor(IceBlue);
	RailBox->SetContent(Rail);
	if (UHorizontalBoxSlot* RailSlot = Row->AddChildToHorizontalBox(RailBox))
	{
		RailSlot->SetPadding(FMargin(0.0f, 4.0f, 12.0f, 4.0f));
		RailSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* CircleBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CircleBox->SetWidthOverride(28.0f);
	CircleBox->SetHeightOverride(28.0f);
	UBorder* Circle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Circle->SetBrushColor(Ring);
	Circle->SetPadding(FMargin(3.0f));
	if (UTexture2D* IconTex = TimelineIconFor(Card.Type))
	{
				UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				Icon->SetBrushFromTexture(IconTex, true);
				Circle->SetContent(Icon);
	}
	else
	{
		UBorder* Fill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Fill->SetBrushColor(CardFill);
		Circle->SetContent(Fill);
	}
	CircleBox->SetContent(Circle);
	if (UHorizontalBoxSlot* CircleSlot = Row->AddChildToHorizontalBox(CircleBox))
	{
		CircleSlot->SetPadding(FMargin(0.0f, 8.0f, 12.0f, 8.0f));
		CircleSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CardBorder->SetBrushColor(CardFill);
	CardBorder->SetPadding(FMargin(12.0f, 8.0f));
	UVerticalBox* TextCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetText(Card.Title);
	TitleText->SetColorAndOpacity(FSlateColor(IceBlue));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Bold", 16)));
	TextCol->AddChildToVerticalBox(TitleText);
	UTextBlock* BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BodyText->SetText(Card.Body);
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.82f, 0.86f, 1.0f)));
	BodyText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Regular", 13)));
	BodyText->SetAutoWrapText(true);
	TextCol->AddChildToVerticalBox(BodyText);
	CardBorder->SetContent(TextCol);
	if (UHorizontalBoxSlot* CardSlot = Row->AddChildToHorizontalBox(CardBorder))
	{
		CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CardSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* RowSlot = VB_Timeline->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}
}

UTexture2D* UEndingWidget::TimelineIconFor(ERunEventType Type) const
{
	switch (Type)
	{
	case ERunEventType::CorrectLift:
	case ERunEventType::WrongLift:
		return LiftIcon;
	case ERunEventType::Ending:
		return EndingIcon;
	default:
		return CallIcon;
	}
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
