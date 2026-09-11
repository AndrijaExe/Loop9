#include "UI/EndingWidget.h"

#include "UI/Loop9WidgetClickBinder.h"
#include "Subsystems/RelationshipSubsystem.h"
#include "Subsystems/Loop9AchievementsSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
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
#include "Widgets/Layout/Anchors.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "Loop9Endings"

namespace
{
	const FLinearColor EndingTextColor = FLinearColor::White;

	void RecolorTextBlocksInTree(UWidget* Root)
	{
		if (!Root)
		{
			return;
		}

		if (UTextBlock* Text = Cast<UTextBlock>(Root))
		{
			Text->SetColorAndOpacity(FSlateColor(EndingTextColor));
			return;
		}

		UUserWidget* AsUserWidget = Cast<UUserWidget>(Root);
		if (!AsUserWidget || !AsUserWidget->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> Children;
		AsUserWidget->WidgetTree->GetAllWidgets(Children);
		for (UWidget* Child : Children)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(Child))
			{
				Text->SetColorAndOpacity(FSlateColor(EndingTextColor));
			}
			else if (UUserWidget* Nested = Cast<UUserWidget>(Child))
			{
				RecolorTextBlocksInTree(Nested);
			}
		}
	}

	void ExpandHitArea(UWidget* Widget, float MinWidth, float MinHeight)
	{
		if (!Widget)
		{
			return;
		}

		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			const FAnchors Anchors = Slot->GetAnchors();
			const bool bStretched =
				!FMath::IsNearlyEqual(Anchors.Minimum.X, Anchors.Maximum.X)
				|| !FMath::IsNearlyEqual(Anchors.Minimum.Y, Anchors.Maximum.Y);
			if (!bStretched)
			{
				Slot->SetAutoSize(false);
				const FVector2D Size = Slot->GetSize();
				Slot->SetSize(FVector2D(
					FMath::Max(Size.X, MinWidth),
					FMath::Max(Size.Y, MinHeight)));
			}
		}

		if (USizeBox* Box = Cast<USizeBox>(Widget))
		{
			Box->ClearMaxDesiredWidth();
			Box->SetMinDesiredWidth(MinWidth);
			Box->SetMinDesiredHeight(MinHeight);
			Box->SetWidthOverride(MinWidth);
			Box->SetHeightOverride(MinHeight);
		}
	}
}

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

	static ConstructorHelpers::FObjectFinder<USoundBase> KeyFinder(
		TEXT("/Game/MyStuff/Sound/UI/TypewriterKey"));
	if (KeyFinder.Succeeded())
	{
		TypingSound = KeyFinder.Object;
	}
}

void UEndingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayoutIfNeeded();
	BindContinueButton();
	RefreshBoundWidgets();
	ApplyTerminalTextColor();
	ExpandContinueButtonHitArea();

	// InitializeEnding usually runs before the widget reaches the viewport, so
	// start here too: the line should begin arriving when the screen appears, and
	// this is also the first point at which a C++ fallback layout has a text block.
	StartTyping();
}

void UEndingWidget::NativeDestruct()
{
	StopTypingTimer();

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

	// The run stats alone read as "the end"; the totals say there is more office
	// to see, which is the difference between a refund and a second run.
	int32 SeenEndings = 1;
	int32 TotalEndings = 6;
	int32 SpottedTypes = 0;
	int32 TotalTypes = 12;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const ULoop9AchievementsSubsystem* Achievements = GameInstance->GetSubsystem<ULoop9AchievementsSubsystem>())
		{
			SeenEndings = FMath::Max(1, Achievements->GetSeenEndingCount());
			TotalEndings = Achievements->GetTotalEndingCount();
			SpottedTypes = Achievements->GetSpottedAnomalyTypeCount();
			TotalTypes = Achievements->GetTotalSpotAnomalyTypeCount();
		}
	}

	EndingStats = FText::Format(
		LOCTEXT("EndingStatsFormat", "Resets: {0}   |   Calls: {1}   |   Endings seen: {2} of {3}   |   Anomalies spotted: {4} of {5}"),
		FText::AsNumber(InResets), FText::AsNumber(InAIInteractions),
		FText::AsNumber(SeenEndings), FText::AsNumber(TotalEndings),
		FText::AsNumber(SpottedTypes), FText::AsNumber(TotalTypes));

	RefreshBoundWidgets();
	StartTyping();
	BP_OnEndingInitialized(EndingType);
}

void UEndingWidget::RequestContinue()
{
	OnContinueRequested.Broadcast();
}

void UEndingWidget::HandleContinueClicked()
{
	// While the line is still arriving the button reads SKIP, so honour that
	// rather than throwing the player back to the menu mid-sentence.
	if (bIsTyping)
	{
		FinishTyping();
		return;
	}

	RequestContinue();
}

void UEndingWidget::StartTyping()
{
	StopTypingTimer();
	FullDescription = EndingDescription.ToString();

	UWorld* World = GetWorld();
	const bool bCanType = bTypeOutDescription && TB_Description && World && !FullDescription.IsEmpty();
	if (!bCanType)
	{
		bIsTyping = false;
		TypedDescription = FullDescription;
		ApplyDescriptionText();
		RefreshContinueButtonLabel();
		return;
	}

	// Same helper the replacement terminal and the phone use, so Dragojlo types
	// the same way here as he does everywhere else, typos and all.
	const float Duration = FullDescription.Len() / FMath::Max(TypingCharactersPerSecond, 1.0f);
	FTypewriterHelper::Begin(TypingState, FullDescription, Duration, TypingTypoProbability);

	TypedDescription.Empty();
	bIsTyping = true;

	ApplyDescriptionText();
	RefreshContinueButtonLabel();
	TypeNextCharacter();
}

void UEndingWidget::StopTypingTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TypingTimerHandle);
	}
	TypingTimerHandle.Invalidate();
}

void UEndingWidget::TypeNextCharacter()
{
	UWorld* World = GetWorld();
	if (!bIsTyping || !World)
	{
		return;
	}

	bool bPlayTypingSound = false;
	const float NextDelay = FTypewriterHelper::Step(TypingState, TypedDescription, bPlayTypingSound);

	ApplyDescriptionText();

	if (bPlayTypingSound && TypingSound)
	{
		UGameplayStatics::PlaySound2D(
			this,
			TypingSound,
			TypingSoundVolume,
			FMath::FRandRange(0.92f, 1.08f));
	}

	if (TypingState.bFinished)
	{
		FinishTyping();
		return;
	}

	World->GetTimerManager().SetTimer(
		TypingTimerHandle, this, &UEndingWidget::TypeNextCharacter, NextDelay, /*bLoop*/ false);
}

void UEndingWidget::FinishTyping()
{
	if (!bIsTyping)
	{
		return;
	}

	StopTypingTimer();
	bIsTyping = false;
	TypedDescription = FullDescription;
	ApplyDescriptionText();
	RefreshContinueButtonLabel();
}

void UEndingWidget::ApplyDescriptionText()
{
	if (!TB_Description)
	{
		return;
	}

	// Trailing caret while typing, so a half-finished line reads as someone still
	// at the keyboard rather than as text that got cut off.
	TB_Description->SetText(FText::FromString(
		bIsTyping ? TypedDescription + TEXT("_") : FullDescription));
}

void UEndingWidget::RefreshContinueButtonLabel()
{
	const FText Label = bIsTyping ? SkipTypingButtonLabel : ContinueButtonLabel;

	if (BT_Continue)
	{
		FLoop9WidgetClickBinder::SetButtonText(BT_Continue, Label);
	}

	if (FallbackContinueButton)
	{
		if (UTextBlock* LabelText = Cast<UTextBlock>(FallbackContinueButton->GetChildAt(0)))
		{
			LabelText->SetText(Label);
			LabelText->SetColorAndOpacity(FSlateColor(EndingTextColor));
		}
	}

	ApplyTerminalTextColor();
	ExpandContinueButtonHitArea();
}

void UEndingWidget::RefreshBoundWidgets()
{
	if (TB_Title)
	{
		TB_Title->SetText(EndingTitle);
		TB_Title->SetColorAndOpacity(FSlateColor(EndingTextColor));
	}

	if (TB_Description)
	{
		if (!bIsTyping)
		{
			FullDescription = EndingDescription.ToString();
			TypedDescription = FullDescription;
		}
		ApplyDescriptionText();
		TB_Description->SetColorAndOpacity(FSlateColor(EndingTextColor));

		TB_Description->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TB_Description->SetRenderOpacity(1.0f);
		TB_Description->SetAutoWrapText(true);
		TB_Description->SetWrapTextAt(0.0f);

		// The six WBP_Ending_* put TB_Description on a Canvas slot with Auto
		// Size on and Auto Wrap off (except Cold Betrayal). Auto-size grows to
		// the unwrapped line, so the blurb leaves the screen. Force a wide
		// box; never reuse a small authored Size.X (that wrapped every word).
		if (UCanvasPanelSlot* DescSlot = Cast<UCanvasPanelSlot>(TB_Description->Slot))
		{
			constexpr float MinWrapWidth = 720.0f;
			constexpr float MinWrapHeight = 160.0f;
			DescSlot->SetAutoSize(false);
			const FVector2D SlotSize = DescSlot->GetSize();
			DescSlot->SetSize(FVector2D(
				FMath::Max(SlotSize.X, MinWrapWidth),
				FMath::Max(SlotSize.Y, MinWrapHeight)));
		}
	}

	if (TB_Stats)
	{
		// Replaced by the session timeline. Keep the widget for layout reuse,
		// but do not show raw Resets | AI interactions.
		TB_Stats->SetVisibility(ESlateVisibility::Collapsed);
	}

	ApplyTerminalTextColor();
	ExpandContinueButtonHitArea();
}

void UEndingWidget::ApplyTerminalTextColor()
{
	RecolorTextBlocksInTree(this);
	RecolorTextBlocksInTree(BT_Continue);
}

void UEndingWidget::ExpandContinueButtonHitArea()
{
	constexpr float MinWidth = 520.0f;
	constexpr float MinHeight = 72.0f;

	ExpandHitArea(BT_Continue, MinWidth, MinHeight);
	ExpandHitArea(FallbackContinueButton, MinWidth, MinHeight);
	FLoop9WidgetClickBinder::AlignToCanvasBottomRight(
		BT_Continue,
		FVector2D(64.0f, 80.0f),
		FVector2D(MinWidth, MinHeight));

	if (UUserWidget* ButtonWidget = Cast<UUserWidget>(BT_Continue))
	{
		TArray<UWidget*> Children;
		if (ButtonWidget->WidgetTree)
		{
			ButtonWidget->WidgetTree->GetAllWidgets(Children);
		}

		for (UWidget* Child : Children)
		{
			ExpandHitArea(Child, MinWidth, MinHeight);
			if (UTextBlock* Label = Cast<UTextBlock>(Child))
			{
				Label->SetAutoWrapText(false);
				Label->SetColorAndOpacity(FSlateColor(EndingTextColor));
			}
		}
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
	TitleText->SetColorAndOpacity(FSlateColor(EndingTextColor));
	TitleText->SetJustification(ETextJustify::Center);
	VBox->AddChildToVerticalBox(TitleText);
	TB_Title = TitleText;

	UTextBlock* DescText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescText"));
	DescText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Regular", 24)));
	DescText->SetColorAndOpacity(FSlateColor(EndingTextColor));
	DescText->SetJustification(ETextJustify::Center);
	DescText->SetAutoWrapText(true);
	DescText->SetWrapTextAt(780.0f);
	VBox->AddChildToVerticalBox(DescText);
	TB_Description = DescText;

	UTextBlock* StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatsText"));
	StatsText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Regular", 18)));
	StatsText->SetColorAndOpacity(FSlateColor(EndingTextColor));
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
	ContinueLabel->SetColorAndOpacity(FSlateColor(EndingTextColor));
	FallbackContinueButton->AddChild(ContinueLabel);
	USizeBox* ContinueSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ContinueSize"));
	ContinueSize->SetMinDesiredWidth(520.0f);
	ContinueSize->SetMinDesiredHeight(72.0f);
	ContinueSize->SetWidthOverride(520.0f);
	ContinueSize->SetHeightOverride(72.0f);
	ContinueSize->SetContent(FallbackContinueButton);
	VBox->AddChildToVerticalBox(ContinueSize);
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
		if (UCanvasPanelSlot* TimelineSlot = Canvas->AddChildToCanvas(Scroll))
		{
			if (TemplateSlot)
			{
				TimelineSlot->SetAnchors(TemplateSlot->GetAnchors());
				TimelineSlot->SetAlignment(TemplateSlot->GetAlignment());
				TimelineSlot->SetAutoSize(false);
				const FVector2D TemplateSize = TemplateSlot->GetSize();
				const float Width = FMath::Max(TemplateSize.X, 640.0f);
				FVector2D Position = TemplateSlot->GetPosition();
				if (TB_Stats && TemplateSlot == Cast<UCanvasPanelSlot>(TB_Stats->Slot))
				{
					TimelineSlot->SetPosition(Position);
				}
				else
				{
					const float OffsetY = FMath::Max(TemplateSize.Y, 72.0f) + 16.0f;
					TimelineSlot->SetPosition(Position + FVector2D(0.0f, OffsetY));
				}
				TimelineSlot->SetSize(FVector2D(Width, 280.0f));
				TimelineSlot->SetZOrder(TemplateSlot->GetZOrder());
			}
			else
			{
				TimelineSlot->SetAnchors(FAnchors(0.08f, 0.42f, 0.62f, 0.82f));
				TimelineSlot->SetOffsets(FMargin(0.0f));
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
	TitleText->SetColorAndOpacity(FSlateColor(EndingTextColor));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Bold", 16)));
	TextCol->AddChildToVerticalBox(TitleText);
	UTextBlock* BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BodyText->SetText(Card.Body);
	BodyText->SetColorAndOpacity(FSlateColor(EndingTextColor));
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
		BT_Continue->SetVisibility(ESlateVisibility::Visible);
	}

	if (FallbackContinueButton)
	{
		FallbackContinueButton->OnClicked.RemoveDynamic(this, &UEndingWidget::HandleContinueClicked);
		FallbackContinueButton->OnClicked.AddDynamic(this, &UEndingWidget::HandleContinueClicked);
		FallbackContinueButton->SetVisibility(ESlateVisibility::Visible);
	}

	// After binding, not before: InitializeEnding can run ahead of NativeConstruct,
	// and the label has to agree with whether the line is still typing.
	RefreshContinueButtonLabel();
}

#undef LOCTEXT_NAMESPACE
