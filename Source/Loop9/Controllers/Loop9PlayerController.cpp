// Copyright Epic Games, Inc. All Rights Reserved.

#include "Loop9PlayerController.h"

#include "Loop9.h"
#include "Loop9CameraManager.h"
#include "Subsystems/AnomalyManager.h"
#include "Subsystems/Loop9LightsSubsystem.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Subsystems/Loop9GameSettingsSubsystem.h"
#include "Subsystems/Loop9TrailerRigSubsystem.h"
#include "Subsystems/Loop9TelemetrySubsystem.h"
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
		// A game sound, not a UI one: pause with the game instead of playing through the pause menu.
		if (UAudioComponent* Sting = UGameplayStatics::SpawnSound2D(this, Sound))
		{
			Sting->bIsUISound = false;
		}
	}
}

UUserWidget* ALoop9PlayerController::GetInteractionPromptWidget() const
{
	return GameplayUI;
}

#if !UE_BUILD_SHIPPING
namespace
{
	/** Every debug command that reaches game state goes through here or GetLoopManager, so this is where the run stops counting as a player's. */
	void TaintTelemetryRun(const UObject* WorldContext)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		if (ULoop9TelemetrySubsystem* Telemetry = GI ? GI->GetSubsystem<ULoop9TelemetrySubsystem>() : nullptr)
		{
			Telemetry->MarkRunTaintedByDebug();
		}
	}

	UAnomalyManager* GetAnomalyManager(const UObject* WorldContext)
	{
		if (!WorldContext)
		{
			return nullptr;
		}
		TaintTelemetryRun(WorldContext);
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
		TaintTelemetryRun(WorldContext);
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

void ALoop9PlayerController::TrailerMark(const FString& Name)
{
#if UE_BUILD_SHIPPING
	(void)Name;
#else
	ULoop9TrailerRigSubsystem* Rig = GetWorld() ? GetWorld()->GetSubsystem<ULoop9TrailerRigSubsystem>() : nullptr;
	if (!Rig)
	{
		DebugScreenMessage(TEXT("TrailerMark: rig not available (start a run first)."), false);
		return;
	}
	// Only the first word is the name; "TrailerMark clip2 pan 25" is a typo, not a mark called that.
	TArray<FString> Tokens;
	Name.ParseIntoArrayWS(Tokens);
	FString Message;
	const bool bOk = Rig->MarkHere(this, Tokens.Num() > 0 ? Tokens[0] : FString(), Message);
	UE_LOG(LogLoop9, Log, TEXT("%s"), *Message);
	DebugScreenMessage(Message, bOk);
#endif
}

void ALoop9PlayerController::TrailerShot(const FString& Args)
{
#if UE_BUILD_SHIPPING
	(void)Args;
#else
	TArray<FString> Tokens;
	Args.ParseIntoArrayWS(Tokens);
	if (Tokens.Num() == 0 || Tokens[0].Equals(TEXT("Help"), ESearchCase::IgnoreCase))
	{
		TrailerHelp();
		return;
	}

	float Pan = 0.0f;
	float Pitch = 0.0f;
	float Dolly = 0.0f;
	float Seconds = 5.0f;
	for (int32 Index = 1; Index + 1 < Tokens.Num(); Index += 2)
	{
		const FString Key = Tokens[Index].ToLower();
		const float Value = FCString::Atof(*Tokens[Index + 1]);
		if (Key == TEXT("pan")) { Pan = Value; }
		else if (Key == TEXT("pitch")) { Pitch = Value; }
		else if (Key == TEXT("dolly")) { Dolly = Value; }
		else if (Key == TEXT("time") || Key == TEXT("sec") || Key == TEXT("seconds")) { Seconds = Value; }
		else
		{
			DebugScreenMessage(FString::Printf(TEXT("TrailerShot: unknown option '%s' (pan / pitch / dolly / time)"), *Tokens[Index]), false);
			return;
		}
	}

	ULoop9TrailerRigSubsystem* Rig = GetWorld() ? GetWorld()->GetSubsystem<ULoop9TrailerRigSubsystem>() : nullptr;
	if (!Rig)
	{
		DebugScreenMessage(TEXT("TrailerShot: rig not available (start a run first)."), false);
		return;
	}
	TaintTelemetryRun(this);
	FString Message;
	const bool bOk = Rig->StartShot(this, Tokens[0], Pan, Pitch, Dolly, Seconds, Message);
	DebugScreenMessage(Message, bOk);
#endif
}

void ALoop9PlayerController::TrailerScene(const FString& Name)
{
#if UE_BUILD_SHIPPING
	(void)Name;
#else
	TArray<FString> Tokens;
	Name.ParseIntoArrayWS(Tokens);
	if (Tokens.Num() == 0)
	{
		TrailerScenes();
		return;
	}
	ULoop9TrailerRigSubsystem* Rig = GetWorld() ? GetWorld()->GetSubsystem<ULoop9TrailerRigSubsystem>() : nullptr;
	if (!Rig)
	{
		DebugScreenMessage(TEXT("TrailerScene: rig not available (start a run first)."), false);
		return;
	}
	TaintTelemetryRun(this);
	FString Message;
	const bool bOk = Rig->StartScene(this, Tokens[0], Message);
	DebugScreenMessage(Message, bOk);
#endif
}

void ALoop9PlayerController::TrailerScenes()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogLoop9, Log, TEXT("Trailer scenes (name — marks it needs — what happens):"));
	FString OnScreen;
	for (const FTrailerScene& Scene : ULoop9TrailerRigSubsystem::GetScenes())
	{
		UE_LOG(LogLoop9, Log, TEXT("  %-14s marks: %-8s %s"), *Scene.Name, *Scene.RequiredMarks, *Scene.Notes);
		OnScreen += FString::Printf(TEXT("%s [%s]  "), *Scene.Name, *Scene.RequiredMarks);
	}
	DebugScreenMessage(OnScreen, true);
#endif
}

void ALoop9PlayerController::TrailerStop()
{
#if !UE_BUILD_SHIPPING
	if (ULoop9TrailerRigSubsystem* Rig = GetWorld() ? GetWorld()->GetSubsystem<ULoop9TrailerRigSubsystem>() : nullptr)
	{
		Rig->StopShot();
		DebugScreenMessage(TEXT("TrailerShot stopped; input is yours again."), true);
	}
#endif
}

void ALoop9PlayerController::TrailerList()
{
#if !UE_BUILD_SHIPPING
	ULoop9TrailerRigSubsystem* Rig = GetWorld() ? GetWorld()->GetSubsystem<ULoop9TrailerRigSubsystem>() : nullptr;
	if (!Rig)
	{
		return;
	}
	const TArray<FString> Names = Rig->ListMarks();
	const FString Joined = Names.Num() > 0 ? FString::Join(Names, TEXT(", ")) : TEXT("(none yet — walk somewhere and TrailerMark <name>)");
	UE_LOG(LogLoop9, Log, TEXT("Trailer marks (%s): %s"), *Rig->GetMarksFilePath(), *Joined);
	DebugScreenMessage(FString::Printf(TEXT("Trailer marks: %s"), *Joined), true);
#endif
}

void ALoop9PlayerController::TrailerHUD(int32 Visible)
{
#if UE_BUILD_SHIPPING
	(void)Visible;
#else
	if (!GameplayUI)
	{
		DebugScreenMessage(TEXT("TrailerHUD: no gameplay HUD on this controller."), false);
		return;
	}
	GameplayUI->SetVisibility(Visible != 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	DebugScreenMessage(Visible != 0 ? TEXT("HUD shown.") : TEXT("HUD hidden. TrailerHUD 1 brings it back."), true);
#endif
}

void ALoop9PlayerController::TrailerHelp()
{
#if !UE_BUILD_SHIPPING
	const TCHAR* Help = TEXT(
		"Trailer capture rig (real gameplay, camera lag/tremor/bob stay on):\n"
		"  TrailerMark <name>                       - save where you stand and where you look\n"
		"  TrailerShot <name> [pan D] [pitch D] [dolly CM] [time S]\n"
		"                                           - teleport to the mark, then an eased move;\n"
		"                                             mouse and movement are ignored until it ends\n"
		"      pan    degrees of yaw, + = right      (default 0)\n"
		"      pitch  degrees, + = up                (default 0)\n"
		"      dolly  cm walked forward, - = back    (default 0, walks at the needed speed)\n"
		"      time   seconds for the move           (default 5; +0.4 s settle, +0.6 s hold)\n"
		"  TrailerScene <name>                      - authored multi-step scene; TrailerScenes lists them\n"
		"                                             and the marks each needs (e.g. hook: look right,\n"
		"                                             back left, and the phone rings on the way back)\n"
		"  TrailerStop                              - abort the move, give input back\n"
		"  TrailerList                              - list saved marks\n"
		"  TrailerHUD 0 | 1                         - hide / show crosshair and prompts\n"
		"Examples: TrailerShot clip2 pan 25 time 5   |   TrailerShot clip1 dolly 150 time 4\n"
		"Marks live in Saved/Trailer/Marks.ini and survive restarts.");
	UE_LOG(LogLoop9, Log, TEXT("%s"), Help);
	DebugScreenMessage(TEXT("TrailerHelp printed to the log (~ console shows it too)."), true);
#endif
}

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
			FString::Printf(TEXT("EndingSetup: unknown ending '%s'. Use 0-5."), *Tokens[0]), false);
		EndingHelp();
		return;
	}

	if (ULoopManagerSubsystem* LoopManager = GetLoopManager(this))
	{
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
		"  EndingHelp\n"
		"After setup the floor is clean, so the DARK elevator is the correct one.\n"
		"Take it to advance; the ending fires at loop 10."));
#endif
}
