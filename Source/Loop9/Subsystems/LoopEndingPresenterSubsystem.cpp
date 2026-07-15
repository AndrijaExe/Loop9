#include "Subsystems/LoopEndingPresenterSubsystem.h"

#include "Subsystems/Loop9AchievementsSubsystem.h"
#include "Subsystems/Loop9TelemetrySubsystem.h"
#include "Subsystems/RelationshipSubsystem.h"
#include "Loop/LoopEndingEvaluator.h"
#include "Loop9GameMode.h"
#include "UI/EndingWidget.h"
#include "UI/ReplacementTerminalWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

namespace
{
	void StartCameraFade(APlayerController* PlayerController, float FromAlpha, float ToAlpha, float Duration)
	{
		if (PlayerController && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, Duration, FLinearColor::Black, false, true);
		}
	}
}

void ULoopEndingPresenterSubsystem::TriggerEndingSequence(URelationshipSubsystem* Relationship)
{
	UWorld* World = GetWorld();
	if (!World || !Relationship)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return;
	}

	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	StartCameraFade(PC, 0.0f, 1.0f, 2.0f);

	const ELoopEndingType EndingType = FLoopEndingEvaluator::Evaluate(Relationship->BuildEndingContext());
	const int32 TotalResets = Relationship->TotalResets;
	const int32 TotalAIInteractions = Relationship->TotalAIInteractions;

	if (ULoop9AchievementsSubsystem* AchievementsSubsystem = GetGameInstance()->GetSubsystem<ULoop9AchievementsSubsystem>())
	{
		AchievementsSubsystem->NotifyRunFinished(EndingType, TotalResets, TotalAIInteractions);
	}

	if (ULoop9TelemetrySubsystem* Telemetry = GetGameInstance()->GetSubsystem<ULoop9TelemetrySubsystem>())
	{
		Telemetry->SendRunFinished(EndingType, TotalResets, TotalAIInteractions);
	}

	FTimerHandle EndingTimer;
	World->GetTimerManager().SetTimer(EndingTimer, [this, PC, EndingType, TotalResets, TotalAIInteractions]()
	{
		ShowEndingWidget(EndingType, TotalResets, TotalAIInteractions);
		PC->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
	}, 2.0f, false);
}

TSubclassOf<UEndingWidget> ULoopEndingPresenterSubsystem::ResolveEndingWidgetClass(ELoopEndingType EndingType) const
{
	if (ALoop9GameMode* LoopGameMode = Cast<ALoop9GameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (const TSubclassOf<UEndingWidget>* MappedClass = LoopGameMode->EndingWidgetClasses.Find(EndingType))
		{
			if (*MappedClass)
			{
				return *MappedClass;
			}
		}

		if (LoopGameMode->EndingWidgetClass)
		{
			return LoopGameMode->EndingWidgetClass;
		}
	}

	return UEndingWidget::StaticClass();
}

void ULoopEndingPresenterSubsystem::ShowEndingWidget(ELoopEndingType EndingType, int32 TotalResets, int32 TotalAIInteractions)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	if (ActiveEndingWidget)
	{
		ActiveEndingWidget->RemoveFromParent();
		ActiveEndingWidget = nullptr;
	}

	const TSubclassOf<UEndingWidget> WidgetClass = ResolveEndingWidgetClass(EndingType);
	UEndingWidget* EndingWidget = CreateWidget<UEndingWidget>(PC, WidgetClass);
	if (!EndingWidget)
	{
		return;
	}

	ActiveEndingWidget = EndingWidget;
	EndingWidget->InitializeEnding(EndingType, TotalResets, TotalAIInteractions);
	EndingWidget->OnContinueRequested.AddDynamic(this, &ULoopEndingPresenterSubsystem::HandleEndingContinueRequested);
	EndingWidget->AddToViewport(2000);

	if (EndingType == ELoopEndingType::TheReplacement)
	{
		FTimerHandle TerminalTimer;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(TerminalTimer, [this]()
			{
				ShowReplacementTerminal();
			}, 3.0f, false);
		}
	}
}

void ULoopEndingPresenterSubsystem::ShowReplacementTerminal()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	TSubclassOf<UReplacementTerminalWidget> WidgetClass = UReplacementTerminalWidget::StaticClass();
	if (ALoop9GameMode* LoopGameMode = Cast<ALoop9GameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (LoopGameMode->ReplacementTerminalWidgetClass)
		{
			WidgetClass = LoopGameMode->ReplacementTerminalWidgetClass;
		}
	}

	UReplacementTerminalWidget* TerminalWidget = CreateWidget<UReplacementTerminalWidget>(PC, WidgetClass);
	if (!TerminalWidget)
	{
		return;
	}

	ActiveTerminalWidget = TerminalWidget;
	TerminalWidget->OnContinueRequested.AddDynamic(this, &ULoopEndingPresenterSubsystem::HandleReplacementTerminalContinueRequested);
	TerminalWidget->AddToViewport(3000);
	TerminalWidget->StartTerminalSequence();
}

void ULoopEndingPresenterSubsystem::HandleEndingContinueRequested()
{
	if (ActiveEndingWidget && ActiveEndingWidget->CurrentEndingType == ELoopEndingType::TheReplacement)
	{
		return;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		ReturnToMainMenu(PC);
	}
}

void ULoopEndingPresenterSubsystem::HandleReplacementTerminalContinueRequested()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		ReturnToMainMenu(PC);
	}
}

void ULoopEndingPresenterSubsystem::ReturnToMainMenu(APlayerController* PlayerController)
{
	UWorld* World = GetWorld();
	if (!World || !PlayerController)
	{
		return;
	}

	if (ActiveEndingWidget)
	{
		ActiveEndingWidget->RemoveFromParent();
		ActiveEndingWidget = nullptr;
	}

	if (ActiveTerminalWidget)
	{
		ActiveTerminalWidget->RemoveFromParent();
		ActiveTerminalWidget = nullptr;
	}

	StartCameraFade(PlayerController, 0.0f, 1.0f, 1.0f);

	FTimerHandle OpenMenuTimer;
	World->GetTimerManager().SetTimer(OpenMenuTimer, [World]()
	{
		UGameplayStatics::OpenLevel(World, FName("MainMenu"));
	}, 1.0f, false);
}
