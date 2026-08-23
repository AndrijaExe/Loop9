#include "UI/HelpWidget.h"

#include "UI/Loop9WidgetClickBinder.h"
#include "UI/MainMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "Loop9Help"

namespace
{
	const FLinearColor HeadingBlue(0.50f, 0.80f, 1.00f, 1.00f);
	const FLinearColor BodyGray(0.82f, 0.84f, 0.86f, 1.00f);
	const FLinearColor ScreenFill(0.02f, 0.04f, 0.06f, 0.96f);
}

void UHelpWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Must run before RebuildWidget() reads WidgetTree->RootWidget, otherwise
	// the layout is captured too late and the screen renders empty.
	BuildFallbackLayoutIfNeeded();
}

void UHelpWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TB_Title)
	{
		TB_Title->SetText(LOCTEXT("HelpTitle", "HOW THE SHIFT WORKS"));
	}

	BindBackButton();
	RefreshSections();
}

void UHelpWidget::NativeDestruct()
{
	if (FallbackBackButton)
	{
		FallbackBackButton->OnClicked.RemoveDynamic(this, &UHelpWidget::HandleBackClicked);
	}

	FLoop9WidgetClickBinder::UnbindClicked(
		BT_Back, this, GET_FUNCTION_NAME_CHECKED(UHelpWidget, HandleBackClicked));

	Super::NativeDestruct();
}

void UHelpWidget::SetReturnTarget(UUserWidget* Target)
{
	ReturnTarget = Target;
}

void UHelpWidget::RequestClose()
{
	OnClosed.Broadcast();
	if (UMainMenuWidget* Menu = Cast<UMainMenuWidget>(ReturnTarget))
	{
		Menu->OnBackFromHelp();
		return;
	}

	RemoveFromParent();
}

void UHelpWidget::HandleBackClicked()
{
	RequestClose();
}

void UHelpWidget::BindBackButton()
{
	if (BT_Back)
	{
		FLoop9WidgetClickBinder::UnbindClicked(
			BT_Back, this, GET_FUNCTION_NAME_CHECKED(UHelpWidget, HandleBackClicked));
		FLoop9WidgetClickBinder::BindClicked(
			BT_Back, this, GET_FUNCTION_NAME_CHECKED(UHelpWidget, HandleBackClicked));
		FLoop9WidgetClickBinder::SetButtonText(BT_Back, LOCTEXT("HelpBack", "BACK"));
	}

	if (FallbackBackButton)
	{
		FallbackBackButton->OnClicked.RemoveDynamic(this, &UHelpWidget::HandleBackClicked);
		FallbackBackButton->OnClicked.AddDynamic(this, &UHelpWidget::HandleBackClicked);
	}
}

void UHelpWidget::BuildFallbackLayoutIfNeeded()
{
	if (VB_Sections || !WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(ScreenFill);
	Backdrop->SetPadding(FMargin(72.0f, 48.0f));
	if (UCanvasPanelSlot* BackdropSlot = Cast<UCanvasPanelSlot>(Root->AddChild(Backdrop)))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
	}

	UVerticalBox* Frame = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Frame"));
	Backdrop->AddChild(Frame);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Bold", 34)));
	TitleText->SetColorAndOpacity(FSlateColor(HeadingBlue));
	if (UVerticalBoxSlot* TitleSlot = Frame->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}
	TB_Title = TitleText;

	// The body outgrows any fixed height once translated, so it scrolls.
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SectionScroll"));
	if (UVerticalBoxSlot* ScrollSlot = Frame->AddChildToVerticalBox(Scroll))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	SB_Sections = Scroll;

	UVerticalBox* Sections = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SectionsBox"));
	Scroll->AddChild(Sections);
	VB_Sections = Sections;

	FallbackBackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
	UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackLabel"));
	BackLabel->SetText(LOCTEXT("HelpBack", "BACK"));
	BackLabel->SetJustification(ETextJustify::Center);
	FallbackBackButton->AddChild(BackLabel);
	if (UVerticalBoxSlot* BackSlot = Frame->AddChildToVerticalBox(FallbackBackButton))
	{
		BackSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
		BackSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void UHelpWidget::RefreshSections()
{
	if (!VB_Sections)
	{
		return;
	}

	VB_Sections->ClearChildren();

	AddSection(VB_Sections,
		LOCTEXT("HelpJobHeading", "YOUR SHIFT"),
		LOCTEXT("HelpJobBody",
			"You are the night technician on the second floor. Every loop puts you back on the same floor of the same office, "
			"and your only job is to decide one thing before you take the elevator: is this floor the way it should be, "
			"or has something changed?"));

	AddSection(VB_Sections,
		LOCTEXT("HelpBaselineHeading", "THE FIRST LOOP IS CLEAN"),
		LOCTEXT("HelpBaselineBody",
			"Nothing is ever wrong on the first loop. Use it. Walk the whole floor, look at the desks, the posters, the "
			"chairs, the lights, and remember how the place is supposed to look. Everything after this is measured "
			"against what you learn here."));

	AddSection(VB_Sections,
		LOCTEXT("HelpAnomalyHeading", "WHAT COUNTS AS AN ANOMALY"),
		LOCTEXT("HelpAnomalyBody",
			"An anomaly is any single thing the office would not do on its own. An object that is missing. An object "
			"standing somewhere it never stood. A poster or a screen whose text has changed. A light that will not stop "
			"flickering, and that you can hear from down the corridor. A door that is locked when it never was. A sound "
			"with nothing making it. Something that is the wrong size. A message on your phone that nobody sent. And, "
			"rarely, something on the floor with you."));

	AddSection(VB_Sections,
		LOCTEXT("HelpElevatorHeading", "THE TWO ELEVATORS"),
		LOCTEXT("HelpElevatorBody",
			"This is the whole game, so read it twice. If you found an anomaly, take the LIT elevator. If the floor is "
			"clean, take the DARK elevator. Get it right and you move up one loop. Get it wrong and you go back to the "
			"beginning. Reaching loop 9 ends your shift, and how you got there decides which ending you get."));

	AddSection(VB_Sections,
		LOCTEXT("HelpPhoneHeading", "THE PHONE"),
		LOCTEXT("HelpPhoneBody",
			"Dragojlo watches the building from another room and answers the phone. He can tell you roughly where to "
			"look, and he is the only company you have down here. He is also a person, not a tool: how you speak to him "
			"changes how much he tells you and how the shift ends. He can be tired, he can be wrong, and if you treat "
			"him badly he can stop being helpful. The decision at the elevator is always yours."));

	AddSection(VB_Sections,
		LOCTEXT("HelpControlsHeading", "CONTROLS"),
		LOCTEXT("HelpControlsBody",
			"W A S D to move, mouse to look, Space to jump.\n"
			"E to interact: answer the phone, open doors, pick an object up and examine it.\n"
			"F to switch your flashlight on and off.\n"
			"Esc to pause, and to put an object down while examining it."));
}

void UHelpWidget::AddSection(UVerticalBox* Parent, const FText& Heading, const FText& Body)
{
	if (!Parent || !WidgetTree)
	{
		return;
	}

	UTextBlock* HeadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HeadingText->SetText(Heading);
	HeadingText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Bold", 20)));
	HeadingText->SetColorAndOpacity(FSlateColor(HeadingBlue));
	if (UVerticalBoxSlot* HeadingSlot = Parent->AddChildToVerticalBox(HeadingText))
	{
		HeadingSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 6.0f));
	}

	UTextBlock* BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BodyText->SetText(Body);
	BodyText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Regular", 15)));
	BodyText->SetColorAndOpacity(FSlateColor(BodyGray));
	BodyText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* BodySlot = Parent->AddChildToVerticalBox(BodyText))
	{
		BodySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}
}

#undef LOCTEXT_NAMESPACE
