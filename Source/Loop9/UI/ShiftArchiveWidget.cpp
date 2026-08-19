#include "UI/ShiftArchiveWidget.h"

#include "UI/MainMenuWidget.h"
#include "Runtime/Loop9RuntimePolicies.h"
#include "Subsystems/Loop9AchievementsSubsystem.h"
#include "UI/Loop9WidgetClickBinder.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "Loop9Archive"

namespace
{
	const FLinearColor UnlockedBlue(0.50f, 0.80f, 1.00f, 1.00f);
	const FLinearColor LockedGray(0.18f, 0.20f, 0.22f, 1.00f);
	const FLinearColor CardFill(0.04f, 0.07f, 0.10f, 0.92f);
}

void UShiftArchiveWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayoutIfNeeded();
	BindBackButton();
	RefreshNodes();
	if (TB_Title)
	{
		TB_Title->SetText(LOCTEXT("ArchiveTitle", "SHIFT ARCHIVE"));
	}
}

void UShiftArchiveWidget::NativeDestruct()
{
	if (FallbackBackButton)
	{
		FallbackBackButton->OnClicked.RemoveDynamic(this, &UShiftArchiveWidget::HandleBackClicked);
	}

	FLoop9WidgetClickBinder::UnbindClicked(
		BT_Back, this, GET_FUNCTION_NAME_CHECKED(UShiftArchiveWidget, HandleBackClicked));

	Super::NativeDestruct();
}

void UShiftArchiveWidget::SetReturnTarget(UUserWidget* Target)
{
	ReturnTarget = Target;
}

void UShiftArchiveWidget::RequestClose()
{
	OnClosed.Broadcast();
	if (UMainMenuWidget* Menu = Cast<UMainMenuWidget>(ReturnTarget))
	{
		Menu->OnBackFromArchive();
		return;
	}

	RemoveFromParent();
}

void UShiftArchiveWidget::HandleBackClicked()
{
	RequestClose();
}

void UShiftArchiveWidget::BindBackButton()
{
	if (BT_Back)
	{
		FLoop9WidgetClickBinder::UnbindClicked(
			BT_Back, this, GET_FUNCTION_NAME_CHECKED(UShiftArchiveWidget, HandleBackClicked));
		FLoop9WidgetClickBinder::BindClicked(
			BT_Back, this, GET_FUNCTION_NAME_CHECKED(UShiftArchiveWidget, HandleBackClicked));
		FLoop9WidgetClickBinder::SetButtonText(BT_Back, LOCTEXT("ArchiveBack", "BACK"));
	}

	if (FallbackBackButton)
	{
		FallbackBackButton->OnClicked.RemoveDynamic(this, &UShiftArchiveWidget::HandleBackClicked);
		FallbackBackButton->OnClicked.AddDynamic(this, &UShiftArchiveWidget::HandleBackClicked);
	}
}

void UShiftArchiveWidget::BuildFallbackLayoutIfNeeded()
{
	if (VB_Nodes || !WidgetTree || WidgetTree->RootWidget)
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
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Bold", 36)));
	TitleText->SetColorAndOpacity(FSlateColor(UnlockedBlue));
	TitleText->SetJustification(ETextJustify::Center);
	VBox->AddChildToVerticalBox(TitleText);
	TB_Title = TitleText;

	UVerticalBox* Nodes = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NodesBox"));
	VBox->AddChildToVerticalBox(Nodes);
	VB_Nodes = Nodes;

	FallbackBackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
	UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackLabel"));
	BackLabel->SetText(LOCTEXT("ArchiveBack", "BACK"));
	BackLabel->SetJustification(ETextJustify::Center);
	FallbackBackButton->AddChild(BackLabel);
	VBox->AddChildToVerticalBox(FallbackBackButton);
}

void UShiftArchiveWidget::RefreshNodes()
{
	if (!VB_Nodes)
	{
		return;
	}

	VB_Nodes->ClearChildren();

	TArray<FString> SeenIds;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoop9AchievementsSubsystem* Achievements = GI->GetSubsystem<ULoop9AchievementsSubsystem>())
		{
			SeenIds = Achievements->GetSeenEndingIds();
		}
	}

	AddNodeRow(VB_Nodes, LOCTEXT("ArchiveHub", "SHIFT"), true, true);

	for (const ELoopEndingType EndingType : Loop9RuntimePolicies::AllEndingTypes())
	{
		const FString AchievementId = ULoop9AchievementsSubsystem::EndingAchievementId(EndingType).ToString();
		const bool bUnlocked = Loop9RuntimePolicies::IsEndingUnlocked(SeenIds, AchievementId);
		const FText Label = bUnlocked
			? TitleForEnding(EndingType)
			: LOCTEXT("ArchiveLocked", "???");
		AddNodeRow(VB_Nodes, Label, bUnlocked, false);
	}
}

void UShiftArchiveWidget::AddNodeRow(UVerticalBox* Parent, const FText& Label, bool bUnlocked, bool bIsHub)
{
	if (!Parent || !WidgetTree)
	{
		return;
	}

	const FLinearColor Accent = bUnlocked ? UnlockedBlue : LockedGray;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	USizeBox* RailBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RailBox->SetWidthOverride(8.0f);
	RailBox->SetHeightOverride(bIsHub ? 28.0f : 22.0f);
	UBorder* Rail = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Rail->SetBrushColor(Accent);
	RailBox->AddChild(Rail);
	if (UHorizontalBoxSlot* RailSlot = Row->AddChildToHorizontalBox(RailBox))
	{
		RailSlot->SetPadding(FMargin(0.0f, 6.0f, 16.0f, 6.0f));
		RailSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* CircleBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CircleBox->SetWidthOverride(bIsHub ? 28.0f : 22.0f);
	CircleBox->SetHeightOverride(bIsHub ? 28.0f : 22.0f);
	UBorder* Circle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Circle->SetBrushColor(Accent);
	CircleBox->AddChild(Circle);
	if (UHorizontalBoxSlot* CircleSlot = Row->AddChildToHorizontalBox(CircleBox))
	{
		CircleSlot->SetPadding(FMargin(0.0f, 4.0f, 16.0f, 4.0f));
		CircleSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrushColor(CardFill);
	Card->SetPadding(FMargin(14.0f, 8.0f));
	UTextBlock* CardText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CardText->SetText(Label);
	CardText->SetColorAndOpacity(FSlateColor(bUnlocked ? UnlockedBlue : FLinearColor(0.45f, 0.45f, 0.48f)));
	CardText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle(bIsHub ? "Bold" : "Regular", 18)));
	Card->AddChild(CardText);
	if (UHorizontalBoxSlot* CardSlot = Row->AddChildToHorizontalBox(Card))
	{
		CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CardSlot->SetVerticalAlignment(VAlign_Center);
	}

	Parent->AddChildToVerticalBox(Row);
}

FText UShiftArchiveWidget::TitleForEnding(ELoopEndingType EndingType)
{
	switch (EndingType)
	{
	case ELoopEndingType::EscapeTogether:
		return LOCTEXT("EscapeTogetherTitle", "ESCAPE TOGETHER");
	case ELoopEndingType::ObedientFool:
		return LOCTEXT("ObedientFoolTitle", "OBEDIENT FOOL");
	case ELoopEndingType::ColdBetrayal:
		return LOCTEXT("ColdBetrayalTitle", "COLD BETRAYAL");
	case ELoopEndingType::ParanoidSurvivor:
		return LOCTEXT("ParanoidSurvivorTitle", "PARANOID SURVIVOR");
	case ELoopEndingType::MergedMemory:
		return LOCTEXT("MergedMemoryTitle", "MERGED MEMORY");
	case ELoopEndingType::TheReplacement:
		return LOCTEXT("TheReplacementTitle", "THE REPLACEMENT");
	default:
		return LOCTEXT("ArchiveLocked", "???");
	}
}

#undef LOCTEXT_NAMESPACE
