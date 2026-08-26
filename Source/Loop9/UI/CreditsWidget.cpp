#include "UI/CreditsWidget.h"

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

#define LOCTEXT_NAMESPACE "Loop9Credits"

namespace
{
	const FLinearColor HeadingBlue(0.50f, 0.80f, 1.00f, 1.00f);
	const FLinearColor BodyGray(0.82f, 0.84f, 0.86f, 1.00f);
	const FLinearColor ScreenFill(0.02f, 0.04f, 0.06f, 0.96f);
}

void UCreditsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildFallbackLayoutIfNeeded();
}

void UCreditsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TB_Title)
	{
		TB_Title->SetText(LOCTEXT("CreditsTitle", "CREDITS"));
	}

	BindBackButton();
	RefreshSections();
}

void UCreditsWidget::NativeDestruct()
{
	if (FallbackBackButton)
	{
		FallbackBackButton->OnClicked.RemoveDynamic(this, &UCreditsWidget::HandleBackClicked);
	}

	FLoop9WidgetClickBinder::UnbindClicked(
		BT_Back, this, GET_FUNCTION_NAME_CHECKED(UCreditsWidget, HandleBackClicked));

	Super::NativeDestruct();
}

void UCreditsWidget::SetReturnTarget(UUserWidget* Target)
{
	ReturnTarget = Target;
}

void UCreditsWidget::RequestClose()
{
	OnClosed.Broadcast();
	if (UMainMenuWidget* Menu = Cast<UMainMenuWidget>(ReturnTarget))
	{
		Menu->OnBackFromCredits();
		return;
	}

	RemoveFromParent();
}

void UCreditsWidget::HandleBackClicked()
{
	RequestClose();
}

void UCreditsWidget::BindBackButton()
{
	if (BT_Back)
	{
		FLoop9WidgetClickBinder::UnbindClicked(
			BT_Back, this, GET_FUNCTION_NAME_CHECKED(UCreditsWidget, HandleBackClicked));
		FLoop9WidgetClickBinder::BindClicked(
			BT_Back, this, GET_FUNCTION_NAME_CHECKED(UCreditsWidget, HandleBackClicked));
		FLoop9WidgetClickBinder::SetButtonText(BT_Back, LOCTEXT("CreditsBack", "BACK"));
	}

	if (FallbackBackButton)
	{
		FallbackBackButton->OnClicked.RemoveDynamic(this, &UCreditsWidget::HandleBackClicked);
		FallbackBackButton->OnClicked.AddDynamic(this, &UCreditsWidget::HandleBackClicked);
	}
}

void UCreditsWidget::BuildFallbackLayoutIfNeeded()
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
	BackLabel->SetText(LOCTEXT("CreditsBack", "BACK"));
	BackLabel->SetJustification(ETextJustify::Center);
	FallbackBackButton->AddChild(BackLabel);
	if (UVerticalBoxSlot* BackSlot = Frame->AddChildToVerticalBox(FallbackBackButton))
	{
		BackSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
		BackSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void UCreditsWidget::RefreshSections()
{
	if (!VB_Sections)
	{
		return;
	}

	VB_Sections->ClearChildren();

	AddSection(VB_Sections,
		LOCTEXT("CreditsDoorsHeading", "DOORS"),
		LOCTEXT("CreditsDoorsBody",
			"\"Door, Front, Closing, A.wav\" by InspectorJ (www.jshaw.co.uk) of Freesound.org. Licensed under CC BY.\n"
			"\"Door Locked\" by BenjaminNelan of Freesound.org (CC0).\n"
			"\"car_door_slam_flat_block_overvecht.wav\" by alfonsseelen of Freesound.org.\n"
			"Door opening generated with Adobe Firefly."));

	AddSection(VB_Sections,
		LOCTEXT("CreditsElevatorHeading", "ELEVATOR"),
		LOCTEXT("CreditsElevatorBody",
			"Button click: OwlishMedia, \"87 Clickety Clips\" (click70.wav), OpenGameArt, CC0.\n"
			"Travel hum: LEGIT Audio, \"The Shop\" fridge drone, OpenGameArt, CC0."));

	AddSection(VB_Sections,
		LOCTEXT("CreditsPhoneHeading", "PHONE"),
		LOCTEXT("CreditsPhoneBody",
			"Ring: \"Phone Ringing #2\" by AUDACITIER of Freesound.org.\n"
			"Pickup: \"Phone Pick Up.wav\" by MootMcnoodles of Freesound.org (CC0).\n"
			"Line cut click: rubberduck, \"100 CC0 SFX\" (switch_01), OpenGameArt, CC0.\n"
			"Line noise: bretbernhoft, \"Frequency Static Sound Effects\" (static4.wav), OpenGameArt, CC0 / Public Domain.\n"
			"Mumbling: \"Mumbling.wav\" by so0rec of Freesound.org.\n"
			"Pursuer murmur: \"Human_Male_Crazy Mumbles_1.wav\" by SilentStrikeZ of Freesound.org."));

	AddSection(VB_Sections,
		LOCTEXT("CreditsFootstepsHeading", "FOOTSTEPS"),
		LOCTEXT("CreditsFootstepsBody",
			"\"Hard Female Footstep (3)\" and \"Hard Female Footstep (4)\" by OwlStorm "
			"(Ashe Kirk / Owlish Media) of Freesound.org (CC0)."));

	AddSection(VB_Sections,
		LOCTEXT("CreditsAmbientHeading", "AMBIENT"),
		LOCTEXT("CreditsAmbientBody",
			"\"Horror ambient.mp3\" by ZHRØ of Freesound.org."));

	AddSection(VB_Sections,
		LOCTEXT("CreditsPursuerHeading", "PURSUER"),
		LOCTEXT("CreditsPursuerBody",
			"Despawn: \"Magic Spell - Whir, and Boom\" by CVLTIV8R of Freesound.org."));

	AddSection(VB_Sections,
		LOCTEXT("CreditsOriginalHeading", "ORIGINAL"),
		LOCTEXT("CreditsOriginalBody",
			"Flashlight click, typewriter key, light flicker, and pursuer tension bed were generated for this game."));
}

void UCreditsWidget::AddSection(UVerticalBox* Parent, const FText& Heading, const FText& Body)
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
