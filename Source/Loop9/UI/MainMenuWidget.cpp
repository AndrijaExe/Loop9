// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Internationalization.h"
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
	EnsureArchiveButton();

	FLoop9WidgetClickBinder::BindClicked(Play, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnPlayClicked));
	FLoop9WidgetClickBinder::BindClicked(Settings, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnSettingsClicked));
	FLoop9WidgetClickBinder::BindClicked(Quit, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnQuitClicked));
	FLoop9WidgetClickBinder::BindClicked(Archive, this, GET_FUNCTION_NAME_CHECKED(UMainMenuWidget, OnArchiveClicked));

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
	if (Archive || !Settings || !WidgetTree)
	{
		return;
	}

	UPanelWidget* Parent = Settings->GetParent();
	if (!Parent)
	{
		return;
	}

	UWidget* NewButton = WidgetTree->ConstructWidget<UWidget>(Settings->GetClass(), TEXT("Archive"));
	if (!NewButton)
	{
		return;
	}

	if (UVerticalBox* VBox = Cast<UVerticalBox>(Parent))
	{
		const int32 SettingsIndex = VBox->GetChildIndex(Settings);
		if (SettingsIndex == INDEX_NONE)
		{
			return;
		}
		UPanelSlot* Inserted = VBox->InsertChildAt(SettingsIndex + 1, NewButton);
		if (UVerticalBoxSlot* NewSlot = Cast<UVerticalBoxSlot>(Inserted))
		{
			if (UVerticalBoxSlot* SettingsSlot = Cast<UVerticalBoxSlot>(Settings->Slot))
			{
				NewSlot->SetPadding(SettingsSlot->GetPadding());
				NewSlot->SetHorizontalAlignment(SettingsSlot->GetHorizontalAlignment());
				NewSlot->SetVerticalAlignment(SettingsSlot->GetVerticalAlignment());
				NewSlot->SetSize(SettingsSlot->GetSize());
			}
		}
	}
	else if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(Parent))
	{
		UCanvasPanelSlot* NewSlot = Canvas->AddChildToCanvas(NewButton);
		UCanvasPanelSlot* SettingsSlot = Cast<UCanvasPanelSlot>(Settings->Slot);
		UCanvasPanelSlot* QuitSlot = Quit ? Cast<UCanvasPanelSlot>(Quit->Slot) : nullptr;
		if (NewSlot && SettingsSlot)
		{
			NewSlot->SetAnchors(SettingsSlot->GetAnchors());
			NewSlot->SetAlignment(SettingsSlot->GetAlignment());
			NewSlot->SetAutoSize(SettingsSlot->GetAutoSize());
			NewSlot->SetSize(SettingsSlot->GetSize());
			NewSlot->SetZOrder(SettingsSlot->GetZOrder());
			if (QuitSlot)
			{
				const FVector2D QuitPos = QuitSlot->GetPosition();
				float DeltaY = QuitPos.Y - SettingsSlot->GetPosition().Y;
				if (FMath::Abs(DeltaY) < 8.0f)
				{
					DeltaY = FMath::Max(SettingsSlot->GetSize().Y, 64.0f) + 8.0f;
				}
				NewSlot->SetPosition(QuitPos);
				QuitSlot->SetPosition(QuitPos + FVector2D(0.0f, DeltaY));
			}
			else
			{
				const float DeltaY = FMath::Max(SettingsSlot->GetSize().Y, 64.0f) + 8.0f;
				NewSlot->SetPosition(SettingsSlot->GetPosition() + FVector2D(0.0f, DeltaY));
			}
		}
	}
	else
	{
		Parent->AddChild(NewButton);
	}

	Archive = NewButton;
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: synthesized Archive button"));
}
