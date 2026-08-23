// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Internationalization.h"
#include "HelpWidget.h"
#include "Loop9WidgetClickBinder.h"
#include "SettingsWidget.h"
#include "ShiftArchiveWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/Widget.h"

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
	EnsureArchiveButton();
	EnsureHelpButton();

	FLoop9WidgetClickBinder::BindClicked(Play, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnPlayClicked));
	FLoop9WidgetClickBinder::BindClicked(Settings, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnSettingsClicked));
	FLoop9WidgetClickBinder::BindClicked(Quit, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnQuitClicked));
	FLoop9WidgetClickBinder::BindClicked(Archive, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnArchiveClicked));
	FLoop9WidgetClickBinder::BindClicked(Help, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnHelpClicked));

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
	FLoop9WidgetClickBinder::SetButtonText(Help, NSLOCTEXT("Loop9Menu", "Help", "HELP"));
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

	Archive = SynthesizeButtonAboveQuit(TEXT("Archive"));
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

	Help = SynthesizeButtonAboveQuit(TEXT("Help"));
	if (Help)
	{
		UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: synthesized Help button above Quit"));
	}
}

UWidget* UMainMenuWidget::SynthesizeButtonAboveQuit(FName ButtonName)
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

	// Anchoring to Quit rather than to a fixed index keeps Quit last however many
	// entries get synthesized, and lets repeated calls stack cleanly: each new
	// button takes Quit's place and pushes Quit one step further down.
	UVerticalBox* StackBox = FindAncestorVerticalBox(Quit ? Quit : Settings);
	if (!StackBox)
	{
		StackBox = FindAncestorVerticalBox(Settings);
	}

	if (StackBox)
	{
		int32 InsertIndex = StackBox->GetChildrenCount();
		if (Quit)
		{
			UWidget* StackChild = Quit;
			while (StackChild && StackChild->GetParent() != StackBox)
			{
				StackChild = StackChild->GetParent();
			}
			const int32 QuitIndex = StackChild ? StackBox->GetChildIndex(StackChild) : INDEX_NONE;
			if (QuitIndex != INDEX_NONE)
			{
				InsertIndex = QuitIndex;
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
	else if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(Quit ? Quit->GetParent() : Settings->GetParent()))
	{
		UCanvasPanelSlot* NewSlot = Canvas->AddChildToCanvas(NewButton);
		UCanvasPanelSlot* QuitSlot = Quit ? Cast<UCanvasPanelSlot>(Quit->Slot) : nullptr;
		UCanvasPanelSlot* SettingsSlot = Cast<UCanvasPanelSlot>(Settings->Slot);
		UCanvasPanelSlot* TemplateSlot = QuitSlot ? QuitSlot : SettingsSlot;
		if (NewSlot && TemplateSlot)
		{
			const float Step = ButtonStackStep(Quit ? Quit : Settings);
			NewSlot->SetAnchors(TemplateSlot->GetAnchors());
			NewSlot->SetAlignment(TemplateSlot->GetAlignment());
			NewSlot->SetAutoSize(TemplateSlot->GetAutoSize());
			NewSlot->SetSize(TemplateSlot->GetSize());
			NewSlot->SetZOrder(TemplateSlot->GetZOrder());
			if (QuitSlot)
			{
				NewSlot->SetPosition(QuitSlot->GetPosition());
				QuitSlot->SetPosition(QuitSlot->GetPosition() + FVector2D(0.0f, Step));
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
