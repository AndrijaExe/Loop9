// Copyright Epic Games, Inc. All Rights Reserved.

#include "Loop9PlayerController.h"

#include "Loop9.h"
#include "Loop9CameraManager.h"
#include "Subsystems/AnomalyManager.h"
#include "Subsystems/Loop9LightsSubsystem.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"
#include "Subsystems/Loop9DragojloMemorySubsystem.h"
#include "UI/BlinkOverlayWidget.h"
#include "UI/SignalBurstWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Loop9Character.h"
#include "HorrorUI.h"
#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
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
			if (ALoop9Character* PossessedCharacter = Cast<ALoop9Character>(InPawn))
			{
				GameplayUI->SetupCharacter(PossessedCharacter);
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

void ALoop9PlayerController::PlaySignalBurst(float Duration)
{
	USignalBurstWidget* Burst = CreateWidget<USignalBurstWidget>(this, USignalBurstWidget::StaticClass());
	if (!Burst)
	{
		return;
	}
	Burst->AddToViewport(5001);
	Burst->Play(Duration);

	USoundBase* Sound = SignalBurstSound;
	if (!Sound)
	{
		Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/MyStuff/Sound/Phone/PhoneLineCut.PhoneLineCut"));
	}
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
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

	/**
	 * Console output only reaches the log, which store reviewers running a
	 * packaged build never see. Mirror the result on screen so a typed command
	 * visibly either took effect or did not.
	 */
	void DebugScreenMessage(const FString& Message, bool bOk)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				INDEX_NONE, 6.0f, bOk ? FColor::Green : FColor::Red, Message);
		}
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
		// A blackout left by an anomaly goes with it.
		if (UWorld* World = GetWorld())
		{
			if (ULoop9LightsSubsystem* Lights = World->GetSubsystem<ULoop9LightsSubsystem>())
			{
				Lights->Restore();
			}
		}
		UE_LOG(LogLoop9, Log, TEXT("AnomalyReset: all anomalies cleared"));
		DebugScreenMessage(TEXT("AnomalyReset: all anomalies cleared"), true);
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
		DebugScreenMessage(
			bOk ? TEXT("AnomalyForceAny: anomaly active on this floor")
				: TEXT("AnomalyForceAny: no inactive anomaly left on this floor"),
			bOk);
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
	DebugScreenMessage(
		bOk ? FString::Printf(TEXT("AnomalyForce '%s': active on this floor. Take the LIT elevator."), *Filter)
			: FString::Printf(TEXT("AnomalyForce '%s': no match on this floor, try another filter."), *Filter),
		bOk);
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

void ALoop9PlayerController::AnomalyHide()
{
	AnomalyForce(TEXT("Hide"));
}

void ALoop9PlayerController::AnomalyMove()
{
	AnomalyForce(TEXT("Move"));
}

void ALoop9PlayerController::AnomalyDoor()
{
	AnomalyForce(TEXT("DoorLock"));
}

void ALoop9PlayerController::AnomalyMaterial()
{
	AnomalyForce(TEXT("MaterialSwap"));
}

void ALoop9PlayerController::AnomalyText()
{
	AnomalyForce(TEXT("Text"));
}

void ALoop9PlayerController::AnomalyScale()
{
	AnomalyForce(TEXT("Scale"));
}

void ALoop9PlayerController::AnomalyPhantom()
{
	AnomalyForce(TEXT("Phantom"));
}

void ALoop9PlayerController::AnomalyLoopNumber()
{
	AnomalyForce(TEXT("LoopNumber"));
}

void ALoop9PlayerController::AnomalyWatcher()
{
	AnomalyForce(TEXT("Watcher"));
}

void ALoop9PlayerController::AnomalyCreep()
{
	AnomalyForce(TEXT("Creep"));
}

void ALoop9PlayerController::PhoneLineRestore()
{
#if !UE_BUILD_SHIPPING
	UAnomalyManager* Manager = GetAnomalyManager(this);
	if (!Manager)
	{
		UE_LOG(LogLoop9, Warning, TEXT("PhoneLineRestore: AnomalyManager not available"));
		return;
	}
	const bool bWasCut = Manager->IsPhoneLineCut();
	Manager->RestorePhoneLine();
	Manager->ForgetPhonesRang();
	UE_LOG(LogLoop9, Log, TEXT("PhoneLineRestore: line %s"), bWasCut ? TEXT("restored") : TEXT("was not cut"));
	DebugScreenMessage(
		bWasCut ? TEXT("PhoneLineRestore: phones work again on this floor. AnomalyPhone to ring them once more.")
				: TEXT("PhoneLineRestore: the line was not cut on this floor."),
		true);
#endif
}

void ALoop9PlayerController::DragojloMemory()
{
#if !UE_BUILD_SHIPPING
	const UGameInstance* GameInstance = GetGameInstance();
	const ULoop9DragojloMemorySubsystem* MemorySubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULoop9DragojloMemorySubsystem>() : nullptr;
	if (!MemorySubsystem)
	{
		UE_LOG(LogLoop9, Warning, TEXT("DragojloMemory: subsystem not available"));
		return;
	}

	const FDragojloMemory& Memory = MemorySubsystem->GetMemory();
	if (Memory.IsEmpty())
	{
		UE_LOG(LogLoop9, Log, TEXT("DragojloMemory: empty (no finished run on this install)"));
		DebugScreenMessage(TEXT("DragojloMemory: empty. He has never met you."), true);
		return;
	}

	const TCHAR* Tone = Memory.LastRunTone > 0 ? TEXT("warm") : (Memory.LastRunTone < 0 ? TEXT("cold") : TEXT("neutral"));
	const FString Summary = FString::Printf(
		TEXT("runs=%d  last=%s  lastCalls=%d  tone=%s  lies=%d  caught=%d  followedHim=%d"),
		Memory.RunsFinished,
		Memory.bHasLastEnding ? *FDragojloMemory::EndingWireLabel(Memory.LastEnding) : TEXT("-"),
		Memory.LastRunCalls,
		Tone,
		Memory.LiesTold,
		Memory.CaughtLying,
		Memory.RunsFollowingHim);
	UE_LOG(LogLoop9, Log, TEXT("DragojloMemory: %s  raw='%s'"), *Summary, *Memory.ToPersistedString());
	DebugScreenMessage(FString::Printf(TEXT("DragojloMemory: %s"), *Summary), true);
#endif
}

void ALoop9PlayerController::DragojloForget()
{
#if !UE_BUILD_SHIPPING
	UGameInstance* GameInstance = GetGameInstance();
	ULoop9DragojloMemorySubsystem* MemorySubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULoop9DragojloMemorySubsystem>() : nullptr;
	if (!MemorySubsystem)
	{
		UE_LOG(LogLoop9, Warning, TEXT("DragojloForget: subsystem not available"));
		return;
	}
	MemorySubsystem->ForgetEverything();
	UE_LOG(LogLoop9, Log, TEXT("DragojloForget: cross-run memory wiped"));
	DebugScreenMessage(TEXT("DragojloForget: he no longer remembers you. Next run sends no run_history."), true);
#endif
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
		"  AnomalyHide                         - force every hide anomaly\n"
		"  AnomalyMove                         - force every move anomaly\n"
		"  AnomalyDoor                         - force every door-lock anomaly\n"
		"  AnomalyMaterial                     - force every material-swap anomaly\n"
		"  AnomalyText                         - force every text anomaly (swaps + notes)\n"
		"  AnomalyScale                        - force every scale anomaly\n"
		"  AnomalyPhantom                      - force every phantom chat message\n"
		"  AnomalyLoopNumber                   - force the loop-counter ? glitch\n"
		"  AnomalyWatcher                      - force the back-turned figure (1.1)\n"
		"  AnomalyCreep                        - force the slow-drift object (1.1)\n"
		"  AnomalyForce <filter> [matIndex]    - force ALL matches by type/class/actor\n"
		"    type is exact; class/actor partial filters require at least 3 characters\n"
		"    filter examples: Hide, Flicker, Audio, Pursuer, Phone, MaterialSwap, Move, Scale, Phantom, DoorLock, LoopNumber, Watcher, Creep, I01\n"
		"    matIndex (optional): 0-based MaterialSwap variant (Die=0, Help=1, ...)\n"
		"  AnomalyAuditMaterials               - list material swaps that would be invisible\n"
		"  PhoneLineRestore                    - undo the ringing-phone line cut on this floor (1.1)\n"
		"  DragojloMemory                      - print what he remembers across runs (1.1)\n"
		"  DragojloForget                      - wipe that memory (1.1)\n"
		"  AudioStatus                         - report why the floor is silent\n"
		"  EndingSetup TheExit                 - arm the ground-floor door (1.1); EndingHelp for the rest\n"
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
			if (Index >= 0 && Index <= static_cast<int32>(ELoopEndingType::TheExit))
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
			{ TEXT("TheExit"), ELoopEndingType::TheExit },
			{ TEXT("Exit"), ELoopEndingType::TheExit },
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
		DebugScreenMessage(
			FString::Printf(TEXT("EndingSetup: unknown ending '%s'. Use 0-6."), *Tokens[0]), false);
		EndingHelp();
		return;
	}

	if (ULoopManagerSubsystem* LoopManager = GetLoopManager(this))
	{
		if (EndingType == ELoopEndingType::TheExit)
		{
			// Not scored: arm the door instead. Walk to the ground-floor exit and open it.
			LoopManager->CurrentLoop = FMath::Max(LoopManager->CurrentLoop, LoopManager->SecretExitMinLoop);
			LoopManager->bDebugSecretExitIgnoresWall = true;
			UE_LOG(LogLoop9, Log, TEXT("EndingSetup TheExit: door armed on loop %d (wall check skipped)."), LoopManager->CurrentLoop);
			DebugScreenMessage(
				TEXT("The Exit armed: the ground-floor street door now accepts the exit\n"
					 "even though the wall is still there. Walk down and open it."),
				true);
			return;
		}

		const bool bOk = LoopManager->ApplyEndingTestSetup(EndingType);
		UE_LOG(LogLoop9, Log, TEXT("EndingSetup %s -> %s. CurrentLoop=9, floor clean, take the DARK elevator."),
			*UEnum::GetValueAsString(EndingType),
			bOk ? TEXT("ok") : TEXT("FAILED predicted mismatch"));
		const FString EndingName = UEnum::GetDisplayValueAsText(EndingType).ToString();
		DebugScreenMessage(
			bOk ? FString::Printf(
					  TEXT("Ending armed: %s. You are on loop 9 and the floor is clean.\n"
						   "Take the DARK elevator to advance and trigger the ending."),
					  *EndingName)
				: FString::Printf(
					  TEXT("EndingSetup %s: state applied but predicts a different ending."),
					  *EndingName),
			bOk);
	}
	else
	{
		UE_LOG(LogLoop9, Warning, TEXT("EndingSetup: LoopManagerSubsystem not available (start PIE first)"));
		DebugScreenMessage(
			TEXT("EndingSetup: start a run from the main menu first."), false);
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
		"  EndingSetup TheExit         (or 6) - arms the ground-floor door instead\n"
		"  EndingHelp\n"
		"After setup the floor is clean, so the DARK elevator is the correct one.\n"
		"Take it to advance; the ending fires at loop 10."));
#endif
}
