// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Internationalization.h"
#include "CreditsWidget.h"
#include "HelpWidget.h"
#include "Loop9WidgetClickBinder.h"
#include "SettingsWidget.h"
#include "ShiftArchiveWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/Widget.h"
#include "HAL/PlatformProcess.h"
#include "Steam/Loop9SteamUtils.h"

namespace
{
	UVerticalBox* FindAncestorVerticalBox(UWidget* Widget)
	{
		for (UWidget* Cursor = Widget ? Widget->GetParent() : nullptr; Cursor; Cursor = Cursor->GetParent())
		{
			if (UVerticalBox* Box = Cast<UVerticalBox>(Cursor))
			{
				return Box;
			}
		}
		return nullptr;
	}

	float ButtonStackStep(UWidget* Widget)
	{
		if (!Widget)
		{
			return 76.0f;
		}

		Widget->ForceLayoutPrepass();
		const float DesiredY = Widget->GetDesiredSize().Y;
		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			return FMath::Max(FMath::Max(DesiredY, CanvasSlot->GetSize().Y), 64.0f) + 12.0f;
		}
		return FMath::Max(DesiredY, 64.0f) + 12.0f;
	}
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	MainMenuGameMode = Cast<AMainMenuGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!MainMenuGameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: Failed to get MainMenuGameMode!"));
	}

	if (!Play)
	{
		Play = GetWidgetFromName(TEXT("Play"));
	}
	if (!Settings)
	{
		Settings = GetWidgetFromName(TEXT("Settings"));
	}
	if (!Quit)
	{
		Quit = GetWidgetFromName(TEXT("Quit"));
	}
	if (!Archive)
	{
		Archive = GetWidgetFromName(TEXT("Archive"));
	}
	if (!Help)
	{
		Help = GetWidgetFromName(TEXT("Help"));
	}
	if (!Credits)
	{
		Credits = GetWidgetFromName(TEXT("Credits"));
	}
	if (!ReportBug)
	{
		ReportBug = GetWidgetFromName(TEXT("ReportBug"));
	}
	// Desired order: Play, How to Play, Settings, Archive, Credits, Quit.
	EnsureHelpButton();
	EnsureArchiveButton();
	EnsureCreditsButton();
	EnsureReportBugButton();
	EnsureStudioBadge();
	EnsureTitleAspect();

	FLoop9WidgetClickBinder::BindClicked(Play, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnPlayClicked));
	FLoop9WidgetClickBinder::BindClicked(Settings, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnSettingsClicked));
	FLoop9WidgetClickBinder::BindClicked(Quit, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnQuitClicked));
	FLoop9WidgetClickBinder::BindClicked(Archive, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnArchiveClicked));
	FLoop9WidgetClickBinder::BindClicked(Help, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnHelpClicked));
	FLoop9WidgetClickBinder::BindClicked(Credits, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnCreditsClicked));
	FLoop9WidgetClickBinder::BindClicked(ReportBug, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnReportBugClicked));

	ApplyLocalizedTexts();

	if (!CultureChangedHandle.IsValid())
	{
		CultureChangedHandle = FInternationalization::Get().OnCultureChanged().AddUObject(
			this, &UMainMenuWidget::HandleCultureChanged);
	}
}

void UMainMenuWidget::NativeDestruct()
{
	FLoop9WidgetClickBinder::UnbindClicked(Play, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnPlayClicked));
	FLoop9WidgetClickBinder::UnbindClicked(Settings, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnSettingsClicked));
	FLoop9WidgetClickBinder::UnbindClicked(Quit, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnQuitClicked));
	FLoop9WidgetClickBinder::UnbindClicked(Archive, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnArchiveClicked));
	FLoop9WidgetClickBinder::UnbindClicked(Help, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnHelpClicked));
	FLoop9WidgetClickBinder::UnbindClicked(Credits, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnCreditsClicked));
	FLoop9WidgetClickBinder::UnbindClicked(ReportBug, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnReportBugClicked));

	if (CultureChangedHandle.IsValid())
	{
		FInternationalization::Get().OnCultureChanged().Remove(CultureChangedHandle);
		CultureChangedHandle.Reset();
	}

	Super::NativeDestruct();
}

void UMainMenuWidget::HandleCultureChanged()
{
	ApplyLocalizedTexts();
}

void UMainMenuWidget::ApplyLocalizedTexts()
{
	FLoop9WidgetClickBinder::SetButtonText(Play, NSLOCTEXT("Loop9Menu", "Play", "PLAY"));
	FLoop9WidgetClickBinder::SetButtonText(Settings, NSLOCTEXT("Loop9Menu", "Settings", "SETTINGS"));
	FLoop9WidgetClickBinder::SetButtonText(Quit, NSLOCTEXT("Loop9Menu", "Quit", "QUIT"));
	FLoop9WidgetClickBinder::SetButtonText(Archive, NSLOCTEXT("Loop9Menu", "Archive", "ARCHIVE"));
	FLoop9WidgetClickBinder::SetButtonText(Help, NSLOCTEXT("Loop9Menu", "HowToPlay", "HOW TO PLAY"));
	FLoop9WidgetClickBinder::SetButtonText(Credits, NSLOCTEXT("Loop9Menu", "Credits", "CREDITS"));
	FLoop9WidgetClickBinder::SetButtonText(ReportBug, NSLOCTEXT("Loop9Menu", "ReportBug", "REPORT A BUG"));
}

void UMainMenuWidget::OnPlayClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Play button clicked"));

	if (MainMenuGameMode)
	{
		MainMenuGameMode->StartGame();
	}
	else
	{
		// Keep the fallback aligned with MainMenuGameMode and packaged maps.
		UGameplayStatics::OpenLevel(GetWorld(), FName("FullOfficeMap"));
	}
}

void UMainMenuWidget::OnSettingsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Settings button clicked"));

	// C++ and Blueprint may both bind the same WBP_Button click — ignore the duplicate.
	if (SettingsWidgetInstance && SettingsWidgetInstance->IsInViewport())
	{
		return;
	}

	if (!SettingsWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: SettingsWidgetClass not set!"));
		return;
	}

	USettingsWidget::RemoveAllFromViewport(GetWorld());

	APlayerController* OwningController = GetOwningPlayer();
	SettingsWidgetInstance = CreateWidget<UUserWidget>(
		OwningController ? OwningController : UGameplayStatics::GetPlayerController(GetWorld(), 0),
		SettingsWidgetClass);
	if (SettingsWidgetInstance)
	{
		if (USettingsWidget* SettingsUI = Cast<USettingsWidget>(SettingsWidgetInstance))
		{
			SettingsUI->SetReturnTarget(this);
		}

		SetVisibility(ESlateVisibility::Hidden);
		SettingsWidgetInstance->AddToViewport(1);
		if (OwningController)
		{
			UWidget* FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(SettingsWidgetInstance);
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			OwningController->SetInputMode(InputMode);
			FocusTarget->SetUserFocus(OwningController);
		}
		UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Settings widget opened"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: Failed to create settings widget!"));
	}
}

void UMainMenuWidget::OnArchiveClicked()
{
	if (ArchiveWidgetInstance && ArchiveWidgetInstance->IsInViewport())
	{
		return;
	}

	const TSubclassOf<UUserWidget> WidgetClass = ArchiveWidgetClass
		? ArchiveWidgetClass
		: TSubclassOf<UUserWidget>(UShiftArchiveWidget::StaticClass());

	APlayerController* OwningController = GetOwningPlayer();
	ArchiveWidgetInstance = CreateWidget<UUserWidget>(
		OwningController ? OwningController : UGameplayStatics::GetPlayerController(GetWorld(), 0),
		WidgetClass);
	if (!ArchiveWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: Failed to create archive widget!"));
		return;
	}

	if (UShiftArchiveWidget* ArchiveUI = Cast<UShiftArchiveWidget>(ArchiveWidgetInstance))
	{
		ArchiveUI->SetReturnTarget(this);
	}

	SetVisibility(ESlateVisibility::Hidden);
	ArchiveWidgetInstance->AddToViewport(1);
	if (OwningController)
	{
		UWidget* FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(ArchiveWidgetInstance);
		if (!FocusTarget)
		{
			FocusTarget = ArchiveWidgetInstance;
		}
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		OwningController->SetInputMode(InputMode);
		FocusTarget->SetUserFocus(OwningController);
	}
}

void UMainMenuWidget::OnBackFromArchive()
{
	if (ArchiveWidgetInstance)
	{
		ArchiveWidgetInstance->RemoveFromParent();
		ArchiveWidgetInstance = nullptr;
	}

	ApplyLocalizedTexts();
	SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* OwningController = GetOwningPlayer())
	{
		UWidget* FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(Archive);
		if (!FocusTarget)
		{
			FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(Play);
		}
		if (!FocusTarget)
		{
			FocusTarget = this;
		}
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		OwningController->SetInputMode(InputMode);
		FocusTarget->SetUserFocus(OwningController);
	}
}

void UMainMenuWidget::OnHelpClicked()
{
	if (HelpWidgetInstance && HelpWidgetInstance->IsInViewport())
	{
		return;
	}

	const TSubclassOf<UUserWidget> WidgetClass = HelpWidgetClass
		? HelpWidgetClass
		: TSubclassOf<UUserWidget>(UHelpWidget::StaticClass());

	APlayerController* OwningController = GetOwningPlayer();
	HelpWidgetInstance = CreateWidget<UUserWidget>(
		OwningController ? OwningController : UGameplayStatics::GetPlayerController(GetWorld(), 0),
		WidgetClass);
	if (!HelpWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: Failed to create help widget!"));
		return;
	}

	if (UHelpWidget* HelpUI = Cast<UHelpWidget>(HelpWidgetInstance))
	{
		HelpUI->SetReturnTarget(this);
	}

	SetVisibility(ESlateVisibility::Hidden);
	HelpWidgetInstance->AddToViewport(1);
	if (OwningController)
	{
		UWidget* FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(HelpWidgetInstance);
		if (!FocusTarget)
		{
			FocusTarget = HelpWidgetInstance;
		}
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		OwningController->SetInputMode(InputMode);
		FocusTarget->SetUserFocus(OwningController);
	}
}

void UMainMenuWidget::OnBackFromHelp()
{
	if (HelpWidgetInstance)
	{
		HelpWidgetInstance->RemoveFromParent();
		HelpWidgetInstance = nullptr;
	}

	ApplyLocalizedTexts();
	SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* OwningController = GetOwningPlayer())
	{
		UWidget* FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(Help);
		if (!FocusTarget)
		{
			FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(Play);
		}
		if (!FocusTarget)
		{
			FocusTarget = this;
		}
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		OwningController->SetInputMode(InputMode);
		FocusTarget->SetUserFocus(OwningController);
	}
}

void UMainMenuWidget::OnCreditsClicked()
{
	if (CreditsWidgetInstance && CreditsWidgetInstance->IsInViewport())
	{
		return;
	}

	const TSubclassOf<UUserWidget> WidgetClass = CreditsWidgetClass
		? CreditsWidgetClass
		: TSubclassOf<UUserWidget>(UCreditsWidget::StaticClass());

	APlayerController* OwningController = GetOwningPlayer();
	CreditsWidgetInstance = CreateWidget<UUserWidget>(
		OwningController ? OwningController : UGameplayStatics::GetPlayerController(GetWorld(), 0),
		WidgetClass);
	if (!CreditsWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget: Failed to create credits widget!"));
		return;
	}

	if (UCreditsWidget* CreditsUI = Cast<UCreditsWidget>(CreditsWidgetInstance))
	{
		CreditsUI->SetReturnTarget(this);
	}

	SetVisibility(ESlateVisibility::Hidden);
	CreditsWidgetInstance->AddToViewport(1);
	if (OwningController)
	{
		UWidget* FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(CreditsWidgetInstance);
		if (!FocusTarget)
		{
			FocusTarget = CreditsWidgetInstance;
		}
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		OwningController->SetInputMode(InputMode);
		FocusTarget->SetUserFocus(OwningController);
	}
}

void UMainMenuWidget::OnBackFromCredits()
{
	if (CreditsWidgetInstance)
	{
		CreditsWidgetInstance->RemoveFromParent();
		CreditsWidgetInstance = nullptr;
	}

	ApplyLocalizedTexts();
	SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* OwningController = GetOwningPlayer())
	{
		UWidget* FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(Credits);
		if (!FocusTarget)
		{
			FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(Play);
		}
		if (!FocusTarget)
		{
			FocusTarget = this;
		}
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		OwningController->SetInputMode(InputMode);
		FocusTarget->SetUserFocus(OwningController);
	}
}

void UMainMenuWidget::OnQuitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Quit button clicked"));

	if (MainMenuGameMode)
	{
		MainMenuGameMode->QuitGame();
	}
	else
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC)
		{
			PC->ConsoleCommand("quit");
		}
	}
}

void UMainMenuWidget::OnBackFromSettings()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Back from settings"));

	USettingsWidget::RemoveAllFromViewport(GetWorld());
	SettingsWidgetInstance = nullptr;

	ApplyLocalizedTexts();
	SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* OwningController = GetOwningPlayer())
	{
		UWidget* FocusTarget = FLoop9WidgetClickBinder::ResolveFocusableWidget(Play);
		if (!FocusTarget)
		{
			FocusTarget = this;
		}
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		OwningController->SetInputMode(InputMode);
		FocusTarget->SetUserFocus(OwningController);
	}
}

void UMainMenuWidget::EnsureArchiveButton()
{
	if (Archive)
	{
		return;
	}

	Archive = SynthesizeButtonBefore(TEXT("Archive"), Quit);
	if (Archive)
	{
		UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: synthesized Archive button above Quit"));
	}
}

void UMainMenuWidget::EnsureHelpButton()
{
	if (Help)
	{
		return;
	}

	Help = SynthesizeButtonBefore(TEXT("Help"), Settings);
	if (Help)
	{
		UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: synthesized How to Play button before Settings"));
	}
}

void UMainMenuWidget::EnsureCreditsButton()
{
	if (Credits)
	{
		return;
	}

	Credits = SynthesizeButtonBefore(TEXT("Credits"), Quit);
	if (Credits)
	{
		UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: synthesized Credits button above Quit"));
	}
}

void UMainMenuWidget::EnsureReportBugButton()
{
	if (ReportBug || !Settings || !WidgetTree)
	{
		return;
	}

	// Same class as the other buttons so it inherits their style, but it goes
	// straight onto the canvas: a corner element, not part of the stack.
	UCanvasPanel* Canvas = nullptr;
	for (UWidget* Walk = Settings->GetParent(); Walk; Walk = Walk->GetParent())
	{
		if (UCanvasPanel* Found = Cast<UCanvasPanel>(Walk))
		{
			Canvas = Found;
			break;
		}
	}

	if (!Canvas)
	{
		// No canvas to pin to: fall back to one more row above Quit.
		ReportBug = SynthesizeButtonBefore(TEXT("ReportBug"), Quit);
		return;
	}

	UWidget* NewButton = WidgetTree->ConstructWidget<UWidget>(Settings->GetClass(), TEXT("ReportBug"));
	if (!NewButton)
	{
		return;
	}

	Canvas->AddChildToCanvas(NewButton);
	FLoop9WidgetClickBinder::AlignToCanvasBottomRight(NewButton, FVector2D(28.0f, 24.0f), FVector2D(300.0f, 60.0f));
	// Shrunk toward the corner so it reads as a footnote, not a seventh menu entry.
	NewButton->SetRenderTransformPivot(FVector2D(1.0f, 1.0f));
	NewButton->SetRenderScale(FVector2D(0.7f, 0.7f));
	NewButton->SetRenderOpacity(0.8f);
	ReportBug = NewButton;
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: synthesized Report a bug button in the corner"));
}

void UMainMenuWidget::EnsureStudioBadge()
{
	if (!WidgetTree || GetWidgetFromName(TEXT("StudioBadge")))
	{
		return;
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	for (UWidget* Walk = Settings ? Settings->GetParent() : nullptr; Walk && !Canvas; Walk = Walk->GetParent())
	{
		Canvas = Cast<UCanvasPanel>(Walk);
	}
	UTexture2D* BadgeTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/MyStuff/UI/Images/SmenaBadge.SmenaBadge"));
	if (!Canvas || !BadgeTexture)
	{
		return;
	}

	UImage* Badge = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("StudioBadge"));
	if (!Badge)
	{
		return;
	}
	Badge->SetBrushFromTexture(BadgeTexture, false);
	Badge->SetVisibility(ESlateVisibility::HitTestInvisible);
	Badge->SetRenderOpacity(0.9f);

	// Mirror of the Report-a-bug corner on the other side: same padding, a
	// studio mark rather than a menu entry.
	UCanvasPanelSlot* BadgeSlot = Canvas->AddChildToCanvas(Badge);
	BadgeSlot->SetAnchors(FAnchors(0.0f, 1.0f));
	BadgeSlot->SetAlignment(FVector2D(0.0f, 1.0f));
	BadgeSlot->SetAutoSize(false);
	BadgeSlot->SetSize(FVector2D(96.0f, 96.0f));
	BadgeSlot->SetPosition(FVector2D(28.0f, -24.0f));
	BadgeSlot->SetZOrder(50);
}

void UMainMenuWidget::EnsureTitleAspect()
{
	if (!WidgetTree)
	{
		return;
	}
	TArray<UWidget*> AllWidgets;
	WidgetTree->GetAllWidgets(AllWidgets);
	for (UWidget* Widget : AllWidgets)
	{
		UImage* Image = Cast<UImage>(Widget);
		const UTexture2D* Texture = Image ? Cast<UTexture2D>(Image->GetBrush().GetResourceObject()) : nullptr;
		if (!Texture || Texture->GetFName() != TEXT("Loop9Title"))
		{
			continue;
		}
		const float TexW = static_cast<float>(Texture->GetSizeX());
		const float TexH = static_cast<float>(Texture->GetSizeY());
		if (TexW <= 0.0f || TexH <= 0.0f)
		{
			return;
		}
		// The WBP stored the size of whatever texture was assigned at design
		// time; keep its width and let the height follow the current texture.
		FSlateBrush Brush = Image->GetBrush();
		const float Width = Brush.ImageSize.X > 0.0f ? Brush.ImageSize.X : TexW;
		Brush.ImageSize = FVector2D(Width, Width * TexH / TexW);
		Image->SetBrush(Brush);
		if (UCanvasPanelSlot* ImageSlot = Cast<UCanvasPanelSlot>(Image->Slot))
		{
			const FVector2D SlotSize = ImageSlot->GetSize();
			if (!ImageSlot->GetAutoSize() && SlotSize.X > 0.0f)
			{
				ImageSlot->SetSize(FVector2D(SlotSize.X, SlotSize.X * TexH / TexW));
			}
		}
		return;
	}
}

void UMainMenuWidget::OnReportBugClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Report a bug clicked -> %s"), *ReportBugUrl);
	if (ReportBugUrl.IsEmpty())
	{
		return;
	}
	if (!FLoop9SteamUtils::OpenWebPage(ReportBugUrl))
	{
		FPlatformProcess::LaunchURL(*ReportBugUrl, nullptr, nullptr);
	}
}

UWidget* UMainMenuWidget::SynthesizeButtonBefore(FName ButtonName, UWidget* Anchor)
{
	if (!Settings || !WidgetTree)
	{
		return nullptr;
	}

	UWidget* NewButton = WidgetTree->ConstructWidget<UWidget>(Settings->GetClass(), ButtonName);
	if (!NewButton)
	{
		return nullptr;
	}

	UWidget* PlaceBefore = Anchor ? Anchor : Quit;
	UVerticalBox* StackBox = FindAncestorVerticalBox(PlaceBefore ? PlaceBefore : Settings);
	if (!StackBox)
	{
		StackBox = FindAncestorVerticalBox(Settings);
	}

	if (StackBox)
	{
		int32 InsertIndex = StackBox->GetChildrenCount();
		if (PlaceBefore)
		{
			UWidget* StackChild = PlaceBefore;
			while (StackChild && StackChild->GetParent() != StackBox)
			{
				StackChild = StackChild->GetParent();
			}
			const int32 AnchorIndex = StackChild ? StackBox->GetChildIndex(StackChild) : INDEX_NONE;
			if (AnchorIndex != INDEX_NONE)
			{
				InsertIndex = AnchorIndex;
			}
		}

		UPanelSlot* Inserted = StackBox->InsertChildAt(InsertIndex, NewButton);
		if (UVerticalBoxSlot* NewSlot = Cast<UVerticalBoxSlot>(Inserted))
		{
			UWidget* StyleSource = Settings;
			if (Settings->GetParent() != StackBox)
			{
				UWidget* StackChild = Settings;
				while (StackChild && StackChild->GetParent() != StackBox)
				{
					StackChild = StackChild->GetParent();
				}
				if (StackChild)
				{
					StyleSource = StackChild;
				}
			}
			if (UVerticalBoxSlot* StyleSlot = Cast<UVerticalBoxSlot>(StyleSource->Slot))
			{
				NewSlot->SetPadding(StyleSlot->GetPadding());
				NewSlot->SetHorizontalAlignment(StyleSlot->GetHorizontalAlignment());
				NewSlot->SetVerticalAlignment(StyleSlot->GetVerticalAlignment());
				NewSlot->SetSize(StyleSlot->GetSize());
			}
		}
	}
	else if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(PlaceBefore ? PlaceBefore->GetParent() : Settings->GetParent()))
	{
		UCanvasPanelSlot* NewSlot = Canvas->AddChildToCanvas(NewButton);
		UCanvasPanelSlot* AnchorSlot = PlaceBefore ? Cast<UCanvasPanelSlot>(PlaceBefore->Slot) : nullptr;
		UCanvasPanelSlot* SettingsSlot = Cast<UCanvasPanelSlot>(Settings->Slot);
		UCanvasPanelSlot* TemplateSlot = AnchorSlot ? AnchorSlot : SettingsSlot;
		if (NewSlot && TemplateSlot)
		{
			const float Step = ButtonStackStep(PlaceBefore ? PlaceBefore : Settings);
			NewSlot->SetAnchors(TemplateSlot->GetAnchors());
			NewSlot->SetAlignment(TemplateSlot->GetAlignment());
			NewSlot->SetAutoSize(TemplateSlot->GetAutoSize());
			NewSlot->SetSize(TemplateSlot->GetSize());
			NewSlot->SetZOrder(TemplateSlot->GetZOrder());
			if (AnchorSlot)
			{
				NewSlot->SetPosition(AnchorSlot->GetPosition());
				AnchorSlot->SetPosition(AnchorSlot->GetPosition() + FVector2D(0.0f, Step));
				if (PlaceBefore != Quit)
				{
					if (UCanvasPanelSlot* QuitSlot = Quit ? Cast<UCanvasPanelSlot>(Quit->Slot) : nullptr)
					{
						QuitSlot->SetPosition(QuitSlot->GetPosition() + FVector2D(0.0f, Step));
					}
				}
			}
			else
			{
				NewSlot->SetPosition(SettingsSlot->GetPosition() + FVector2D(0.0f, Step));
			}
		}
	}
	else if (UPanelWidget* Parent = Settings->GetParent())
	{
		Parent->AddChild(NewButton);
	}

	return NewButton;
}
