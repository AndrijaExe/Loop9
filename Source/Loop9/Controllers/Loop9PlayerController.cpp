// Copyright Epic Games, Inc. All Rights Reserved.

#include "Loop9PlayerController.h"

#include "Loop9.h"
#include "Loop9CameraManager.h"
#include "Subsystems/AnomalyManager.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"
#include "UI/BlinkOverlayWidget.h"
#include "HorrorCharacter.h"
#include "HorrorUI.h"
#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Loop/LoopTypes.h"
#include "Loop9GameMode.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundBase.h"

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

void ALoop9PlayerController::AnomalyFlicker()
{
	AnomalyForce(TEXT("Flicker"));
}

void ALoop9PlayerController::AnomalyPhone()
{
	AnomalyForce(TEXT("Audio"));
}

void ALoop9PlayerController::AnomalyPursuer()
{
	AnomalyForce(TEXT("Pursuer"));
}

void ALoop9PlayerController::AnomalyMove()
{
	AnomalyForce(TEXT("Move"));
}

void ALoop9PlayerController::AnomalyMaterial()
{
	AnomalyForce(TEXT("MaterialSwap"));
}

void ALoop9PlayerController::AnomalyAuditMaterials()
{
#if !UE_BUILD_SHIPPING
	if (UAnomalyManager* Manager = GetAnomalyManager(this))
	{
		Manager->AuditMaterialAnomalies();
	}
	else
	{
		UE_LOG(LogLoop9, Warning, TEXT("AnomalyAuditMaterials: AnomalyManager not available"));
	}
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
		"  AnomalyFlicker                      - force every light-flicker anomaly\n"
		"  AnomalyPhone                        - force every phone / audio anomaly\n"
		"  AnomalyPursuer                      - force the pursuer\n"
		"  AnomalyMove                         - force every move anomaly\n"
		"  AnomalyMaterial                     - force every material-swap anomaly\n"
		"  AnomalyForce <filter> [matIndex]    - force ALL matches by type/class/actor\n"
		"    type is exact; class/actor partial filters require at least 3 characters\n"
		"    filter examples: Flicker, Audio, Pursuer, Phone, MaterialSwap, Move, I01\n"
		"    matIndex (optional): 0-based MaterialSwap variant (Die=0, Help=1, ...)\n"
		"  AnomalyAuditMaterials               - list material swaps that would be invisible\n"
		"  AudioStatus                         - report why the floor is silent\n"
		"  AnomalyHelp                         - this message"));
#endif
}

void ALoop9PlayerController::AudioStatus()
{
#if !UE_BUILD_SHIPPING
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const ULoop9GameSettingsSubsystem* Settings = nullptr;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		Settings = GameInstance->GetSubsystem<ULoop9GameSettingsSubsystem>();
	}

	UE_LOG(LogLoop9, Log, TEXT("Audio status: device=%s master=%.2f ambient=%.2f"),
		World->GetAudioDeviceRaw() ? TEXT("yes") : TEXT("NO"),
		Settings ? Settings->GetMasterVolume() : -1.0f,
		Settings ? Settings->GetAmbientVolume() : -1.0f);

	if (const ALoop9GameMode* GameMode = Cast<ALoop9GameMode>(World->GetAuthGameMode()))
	{
		UE_LOG(LogLoop9, Log, TEXT("  music bed: sound=%s volume=%.2f playing=%s"),
			GameMode->LevelMusicSound ? *GameMode->LevelMusicSound->GetName() : TEXT("NONE"),
			GameMode->LevelMusicVolume,
			GameMode->IsLevelMusicPlaying() ? TEXT("yes") : TEXT("NO"));
	}
	else
	{
		UE_LOG(LogLoop9, Log, TEXT("  music bed: game mode is not ALoop9GameMode"));
	}

	int32 AmbientCount = 0;
	for (TActorIterator<AAmbientSound> It(World); It; ++It)
	{
		++AmbientCount;
		const UAudioComponent* Audio = It->GetAudioComponent();
		UE_LOG(LogLoop9, Log, TEXT("  ambient '%s': sound=%s playing=%s"),
			*It->GetActorNameOrLabel(),
			(Audio && Audio->Sound) ? *Audio->Sound->GetName() : TEXT("NONE"),
			(Audio && Audio->IsPlaying()) ? TEXT("yes") : TEXT("no"));
	}

	UE_LOG(LogLoop9, Log, TEXT("  %d AmbientSound actor(s) placed in the map"), AmbientCount);
#endif
}

#if !UE_BUILD_SHIPPING
namespace
{
	bool ParseEndingTypeArg(const FString& Token, ELoopEndingType& OutType)
	{
		if (Token.IsNumeric())
		{
			const int32 Index = FCString::Atoi(*Token);
			if (Index >= 0 && Index <= static_cast<int32>(ELoopEndingType::TheReplacement))
			{
				OutType = static_cast<ELoopEndingType>(Index);
				return true;
			}
			return false;
		}

		const FString Normalized = Token.Replace(TEXT(" "), TEXT("")).Replace(TEXT("_"), TEXT(""));
		static const TPair<const TCHAR*, ELoopEndingType> Aliases[] = {
			{ TEXT("EscapeTogether"), ELoopEndingType::EscapeTogether },
			{ TEXT("ObedientFool"), ELoopEndingType::ObedientFool },
			{ TEXT("ColdBetrayal"), ELoopEndingType::ColdBetrayal },
			{ TEXT("ParanoidSurvivor"), ELoopEndingType::ParanoidSurvivor },
			{ TEXT("MergedMemory"), ELoopEndingType::MergedMemory },
			{ TEXT("TheReplacement"), ELoopEndingType::TheReplacement },
			{ TEXT("Replacement"), ELoopEndingType::TheReplacement },
		};

		for (const TPair<const TCHAR*, ELoopEndingType>& Alias : Aliases)
		{
			if (Normalized.Equals(Alias.Key, ESearchCase::IgnoreCase))
			{
				OutType = Alias.Value;
				return true;
			}
		}

		return false;
	}

	ULoopManagerSubsystem* GetLoopManager(const UObject* WorldContext)
	{
		if (!WorldContext)
		{
			return nullptr;
		}
		if (const UWorld* World = WorldContext->GetWorld())
		{
			if (UGameInstance* GI = World->GetGameInstance())
			{
				return GI->GetSubsystem<ULoopManagerSubsystem>();
			}
		}
		return nullptr;
	}
}
#endif

void ALoop9PlayerController::EndingSetup(const FString& Args)
{
#if UE_BUILD_SHIPPING
	(void)Args;
#else
	TArray<FString> Tokens;
	Args.ParseIntoArrayWS(Tokens);
	if (Tokens.Num() == 0 || Tokens[0].Equals(TEXT("Help"), ESearchCase::IgnoreCase))
	{
		EndingHelp();
		return;
	}

	ELoopEndingType EndingType = ELoopEndingType::EscapeTogether;
	if (!ParseEndingTypeArg(Tokens[0], EndingType))
	{
		UE_LOG(LogLoop9, Warning, TEXT("EndingSetup: unknown ending '%s'"), *Tokens[0]);
		EndingHelp();
		return;
	}

	if (ULoopManagerSubsystem* LoopManager = GetLoopManager(this))
	{
		const bool bOk = LoopManager->ApplyEndingTestSetup(EndingType);
		UE_LOG(LogLoop9, Log, TEXT("EndingSetup %s -> %s. CurrentLoop=9. Press ADVANCE (no anomaly)."),
			*UEnum::GetValueAsString(EndingType),
			bOk ? TEXT("ok") : TEXT("FAILED predicted mismatch"));
	}
	else
	{
		UE_LOG(LogLoop9, Warning, TEXT("EndingSetup: LoopManagerSubsystem not available (start PIE first)"));
	}
#endif
}

void ALoop9PlayerController::EndingHelp()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogLoop9, Log, TEXT(
		"Ending debug commands:\n"
		"  EndingSetup EscapeTogether   (or 0) - first ending, loop 9\n"
		"  EndingSetup ObedientFool     (or 1)\n"
		"  EndingSetup ColdBetrayal    (or 2)\n"
		"  EndingSetup ParanoidSurvivor (or 3)\n"
		"  EndingSetup MergedMemory    (or 4)\n"
		"  EndingSetup TheReplacement  (or 5)\n"
		"  EndingHelp\n"
		"After setup: take elevator ADVANCE (anomalies cleared). Ending fires at loop 10."));
#endif
}
