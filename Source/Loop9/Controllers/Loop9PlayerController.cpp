// Copyright Epic Games, Inc. All Rights Reserved.

#include "Loop9PlayerController.h"

#include "Loop9.h"
#include "Loop9CameraManager.h"
#include "Subsystems/AnomalyManager.h"
#include "Subsystems/Loop9GameplayNotificationSubsystem.h"
#include "UI/BlinkOverlayWidget.h"
#include "HorrorCharacter.h"
#include "HorrorUI.h"
#include "Engine/GameInstance.h"

ALoop9PlayerController::ALoop9PlayerController()
{
	PlayerCameraManagerClass = ALoop9CameraManager::StaticClass();
}

void ALoop9PlayerController::BeginPlay()
{
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetIgnoreLookInput(false);

	Super::BeginPlay();
}

void ALoop9PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (GameplayUIClass)
	{
		if (!GameplayUI)
		{
			GameplayUI = CreateWidget<UHorrorUI>(this, GameplayUIClass);
			if (GameplayUI)
			{
				GameplayUI->AddToViewport(0);
			}
		}

		if (GameplayUI)
		{
			if (AHorrorCharacter* HorrorCharacter = Cast<AHorrorCharacter>(InPawn))
			{
				GameplayUI->SetupCharacter(HorrorCharacter);
			}

			GameplayUI->SetCrosshairVisible(true);
			ClearInteractionPrompt();
		}
	}
}

void ALoop9PlayerController::ShowGameplayNotification(const FText& Message, float DisplayDuration)
{
	if (ULoop9GameplayNotificationSubsystem* Notifications = ULoop9GameplayNotificationSubsystem::GetGameplayNotifications(this))
	{
		Notifications->AddMessage(Message, DisplayDuration);
	}
}

void ALoop9PlayerController::CreateBlinkOverlay()
{
	if (BlinkOverlayInstance)
	{
		return;
	}

	if (!BlinkOverlayWidgetClass)
	{
		UE_LOG(LogLoop9, Warning, TEXT("BlinkOverlayWidgetClass is not set on Loop9PlayerController."));
		return;
	}

	BlinkOverlayInstance = CreateWidget<UBlinkOverlayWidget>(this, BlinkOverlayWidgetClass);
	if (BlinkOverlayInstance)
	{
		BlinkOverlayInstance->AddToViewport(5000);
	}
}

void ALoop9PlayerController::PlayBlink(float Duration)
{
	if (!BlinkOverlayInstance)
	{
		CreateBlinkOverlay();
	}

	if (BlinkOverlayInstance)
	{
		BlinkOverlayInstance->PlayBlink(Duration);
	}
}

void ALoop9PlayerController::RemoveBlinkOverlay()
{
	if (BlinkOverlayInstance)
	{
		BlinkOverlayInstance->RemoveFromParent();
		BlinkOverlayInstance = nullptr;
	}
}

UUserWidget* ALoop9PlayerController::GetInteractionPromptWidget() const
{
	return GameplayUI;
}

#if !UE_BUILD_SHIPPING
namespace
{
	UAnomalyManager* GetAnomalyManager(const UObject* WorldContext)
	{
		if (!WorldContext)
		{
			return nullptr;
		}
		if (const UWorld* World = WorldContext->GetWorld())
		{
			if (UGameInstance* GI = World->GetGameInstance())
			{
				return GI->GetSubsystem<UAnomalyManager>();
			}
		}
		return nullptr;
	}
}
#endif

void ALoop9PlayerController::AnomalyList()
{
#if !UE_BUILD_SHIPPING
	if (UAnomalyManager* Manager = GetAnomalyManager(this))
	{
		Manager->PrintAnomalyStats();
	}
	else
	{
		UE_LOG(LogLoop9, Warning, TEXT("AnomalyList: AnomalyManager not available"));
	}
#endif
}

void ALoop9PlayerController::AnomalyReset()
{
#if !UE_BUILD_SHIPPING
	if (UAnomalyManager* Manager = GetAnomalyManager(this))
	{
		Manager->ResetAllAnomalies();
		UE_LOG(LogLoop9, Log, TEXT("AnomalyReset: all anomalies cleared"));
	}
#endif
}

void ALoop9PlayerController::AnomalyForceAny()
{
#if !UE_BUILD_SHIPPING
	if (UAnomalyManager* Manager = GetAnomalyManager(this))
	{
		const bool bOk = Manager->ForceActivateAnyAnomaly();
		UE_LOG(LogLoop9, Log, TEXT("AnomalyForceAny: %s"), bOk ? TEXT("ok") : TEXT("failed"));
	}
#endif
}

void ALoop9PlayerController::AnomalyForce(const FString& Args)
{
#if UE_BUILD_SHIPPING
	(void)Args;
#else
	UAnomalyManager* Manager = GetAnomalyManager(this);
	if (!Manager)
	{
		UE_LOG(LogLoop9, Warning, TEXT("AnomalyForce: AnomalyManager not available"));
		return;
	}

	TArray<FString> Tokens;
	Args.ParseIntoArrayWS(Tokens);
	if (Tokens.Num() == 0)
	{
		AnomalyHelp();
		return;
	}

	const FString& Filter = Tokens[0];
	int32 MaterialIndex = INDEX_NONE;
	if (Tokens.Num() >= 2 && Tokens[1].IsNumeric())
	{
		MaterialIndex = FCString::Atoi(*Tokens[1]);
	}

	const bool bOk = Manager->ForceActivateByFilter(Filter, MaterialIndex);
	UE_LOG(LogLoop9, Log, TEXT("AnomalyForce '%s' mat=%d -> %s"),
		*Filter, MaterialIndex, bOk ? TEXT("ok") : TEXT("failed"));
#endif
}

void ALoop9PlayerController::AnomalyHelp()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogLoop9, Log, TEXT(
		"Anomaly debug commands:\n"
		"  AnomalyList                         - list all registered anomalies\n"
		"  AnomalyReset                        - clear all active anomalies\n"
		"  AnomalyForceAny                     - force one random inactive anomaly\n"
		"  AnomalyForce <filter> [matIndex]    - force ALL matches by type/class/actor\n"
		"    type is exact; class/actor partial filters require at least 3 characters\n"
		"    filter examples: MaterialSwap, Text, Move, OldMagazine, I01, F01, D01\n"
		"    matIndex (optional): 0-based MaterialSwap variant (Die=0, Help=1, ...)\n"
		"  AnomalyHelp                         - this message"));
#endif
}
