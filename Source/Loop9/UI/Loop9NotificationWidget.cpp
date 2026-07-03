#include "UI/Loop9NotificationWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

void ULoop9NotificationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayoutIfNeeded();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULoop9NotificationWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
		World->GetTimerManager().ClearTimer(GapTimerHandle);
	}

	Super::NativeDestruct();
}

void ULoop9NotificationWidget::AddMessage(const FText& Message, float TimeDuration)
{
	ShowNotification(Message, TimeDuration);
}

void ULoop9NotificationWidget::ShowNotification(const FText& Message, float DisplayDuration)
{
	if (Message.IsEmpty())
	{
		return;
	}

	FLoop9NotificationEntry Entry;
	Entry.Message = Message;
	Entry.DisplayDuration = DisplayDuration > 0.0f ? DisplayDuration : DefaultDisplayDuration;
	PendingNotifications.Add(Entry);

	if (!bIsShowingNotification)
	{
		DisplayNextNotification();
	}
}

void ULoop9NotificationWidget::ClearNotificationQueue()
{
	PendingNotifications.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
		World->GetTimerManager().ClearTimer(GapTimerHandle);
	}

	if (bIsShowingNotification)
	{
		bIsShowingNotification = false;
		BP_OnNotificationHidden();
	}

	if (TB_Message)
	{
		TB_Message->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (FallbackMessageText)
	{
		FallbackMessageText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULoop9NotificationWidget::DisplayNextNotification()
{
	if (PendingNotifications.Num() == 0)
	{
		return;
	}

	const FLoop9NotificationEntry Entry = PendingNotifications[0];
	PendingNotifications.RemoveAt(0);

	bIsShowingNotification = true;
	RefreshMessageText(Entry.Message);
	BP_OnNotificationShown(Entry.Message, Entry.DisplayDuration);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HideTimerHandle,
			this,
			&ULoop9NotificationWidget::FinishCurrentNotification,
			Entry.DisplayDuration,
			false);
	}
}

void ULoop9NotificationWidget::FinishCurrentNotification()
{
	bIsShowingNotification = false;
	BP_OnNotificationHidden();

	if (TB_Message)
	{
		TB_Message->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (FallbackMessageText)
	{
		FallbackMessageText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (PendingNotifications.Num() == 0 || !GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		GapTimerHandle,
		this,
		&ULoop9NotificationWidget::DisplayNextNotification,
		MessageGapDuration,
		false);
}

void ULoop9NotificationWidget::RefreshMessageText(const FText& Message)
{
	if (TB_Message)
	{
		TB_Message->SetText(Message);
		TB_Message->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (FallbackMessageText)
	{
		FallbackMessageText->SetText(Message);
		FallbackMessageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void ULoop9NotificationWidget::BuildFallbackLayoutIfNeeded()
{
	if (TB_Message || !WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NotificationRoot"));
	WidgetTree->RootWidget = Root;

	FallbackMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NotificationMessage"));
	FallbackMessageText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Bold", 22)));
	FallbackMessageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f)));
	FallbackMessageText->SetJustification(ETextJustify::Center);
	FallbackMessageText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	FallbackMessageText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	FallbackMessageText->SetVisibility(ESlateVisibility::Collapsed);

	if (UCanvasPanelSlot* MessageSlot = Cast<UCanvasPanelSlot>(Root->AddChild(FallbackMessageText)))
	{
		MessageSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		MessageSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		MessageSlot->SetPosition(FVector2D(0.0f, -120.0f));
		MessageSlot->SetAutoSize(true);
	}
}
