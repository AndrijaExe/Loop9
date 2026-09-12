#include "Subsystems/Loop9TrailerRigSubsystem.h"

#include "Anomaly/AudioAnomalyComponent.h"
#include "Anomaly/Pursuer/PursuerAnomalyCharacter.h"
#include "Anomaly/Pursuer/PursuerAnomalyComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Loop9.h"
#include "Loop9GameMode.h"
#include "Controllers/Loop9PlayerController.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Subsystems/AnomalyManager.h"
#include "Subsystems/Loop9RingingFloorSubsystem.h"
#include "Subsystems/Loop9TelemetrySubsystem.h"
#include "UObject/UObjectIterator.h"

namespace
{
	/** Lets the camera lag catch up on a teleport before the move starts. */
	constexpr float TeleportSettleSeconds = 0.4f;
	/** Keeps the frame still after the last step so the cut has a clean tail. */
	constexpr float HoldSeconds = 0.6f;
	/** Fade to black on the phone pickup, and the fade back once the shot ends. */
	constexpr float PickupFadeSeconds = 0.8f;
	constexpr float FadeBackSeconds = 0.6f;
	/** A rig Pursuer never gets closer than this, or the catch logic despawns him. */
	constexpr float PursuerMinDistanceCm = 170.0f;
	const TCHAR* MarksSection = TEXT("Marks");
	const TCHAR* PickupSoundPath = TEXT("/Game/MyStuff/Sound/Phone/PhonePickup.PhonePickup");

	FTrailerStep Step(const TCHAR* Mark, float Pan, float Pitch, float Dolly, float Seconds, const TCHAR* Action = TEXT(""), float ActionAt = 0.0f)
	{
		FTrailerStep S;
		S.Mark = Mark;
		S.PanDegrees = Pan;
		S.PitchDegrees = Pitch;
		S.DollyCm = Dolly;
		S.Seconds = Seconds;
		S.Action = Action;
		S.ActionAtAlpha = ActionAt;
		return S;
	}
}

const TArray<FTrailerScene>& ULoop9TrailerRigSubsystem::GetScenes()
{
	// Authored once here so a shot list is a set of TrailerMark names, not a
	// set of mouse movements. Marks are the only thing set in the map.
	static const TArray<FTrailerScene> Scenes = {
		{ TEXT("hook"), TEXT("hook"),
		  TEXT("Open-space, 5+ lamps and the ringing phone in view. Look right, back left, and on the way right again the phone rings and the floor goes dark."),
		  { Step(TEXT("hook"), 0.0f, 0.0f, 0.0f, 2.0f),
		    Step(TEXT(""), 50.0f, 0.0f, 0.0f, 1.6f),
		    Step(TEXT(""), -100.0f, 0.0f, 0.0f, 2.0f),
		    Step(TEXT(""), 50.0f, 0.0f, 0.0f, 1.6f, TEXT("Phone"), 0.35f),
		    Step(TEXT(""), 0.0f, 0.0f, 0.0f, 3.0f) } },

		{ TEXT("lifts"), TEXT("lifts"),
		  TEXT("Both elevators in frame, lit and dark. Slow walk toward them."),
		  { Step(TEXT("lifts"), 0.0f, 0.0f, 90.0f, 3.5f) } },

		{ TEXT("desk"), TEXT("desk"),
		  TEXT("Baseline: slow pan right across the desks. Record on a clean floor."),
		  { Step(TEXT("desk"), 25.0f, 0.0f, 0.0f, 4.0f) } },

		{ TEXT("desk_creep"), TEXT("desk"),
		  TEXT("Same pan with Creep: forces it, waits 55 s for the object to travel, then the identical pan. Cut the wait."),
		  { Step(TEXT("desk"), 0.0f, 0.0f, 0.0f, 0.5f, TEXT("Creep"), 0.0f),
		    Step(TEXT(""), 0.0f, 0.0f, 0.0f, 55.0f),
		    Step(TEXT(""), 25.0f, 0.0f, 0.0f, 4.0f) } },

		{ TEXT("shelf"), TEXT("shelf"),
		  TEXT("Facing a shelf/desk with a hideable object. Hide is forced, then a slow walk in with a slight look down."),
		  { Step(TEXT("shelf"), 0.0f, 0.0f, 0.0f, 1.0f, TEXT("Hide"), 0.0f),
		    Step(TEXT(""), 0.0f, -8.0f, 120.0f, 3.0f) } },

		{ TEXT("hall"), TEXT("hall"),
		  TEXT("Corridor under a ceiling light, camera a touch low. Flicker forced, then hold with a slight tilt up."),
		  { Step(TEXT("hall"), 0.0f, 0.0f, 0.0f, 1.0f, TEXT("Flicker"), 0.0f),
		    Step(TEXT(""), 0.0f, 6.0f, 0.0f, 4.0f) } },

		{ TEXT("watcher"), TEXT("watcher"),
		  TEXT("Mark 5+ m from the Watcher anchor, facing it. He appears, then a slow walk that stops short of him."),
		  { Step(TEXT("watcher"), 0.0f, 0.0f, 0.0f, 1.5f, TEXT("Watcher"), 0.0f),
		    Step(TEXT(""), 0.0f, 0.0f, 150.0f, 4.0f) } },

		{ TEXT("watcher_burst"), TEXT("watcher"),
		  TEXT("Same mark, walk all the way in: signal burst and blackout. Cut on the burst."),
		  { Step(TEXT("watcher"), 0.0f, 0.0f, 0.0f, 1.5f, TEXT("Watcher"), 0.0f),
		    Step(TEXT(""), 0.0f, 0.0f, 420.0f, 3.5f) } },

		{ TEXT("pursuer"), TEXT("pursuer, pursuer_far, pursuer_near"),
		  TEXT("No anomaly. 'pursuer' = where you stand and look; 'pursuer_far' = where he stands in that view (6+ m); 'pursuer_near' = closer on the same line (2+ m). He appears, a 1.2 s glance right and back, and he is nearer."),
		  { Step(TEXT("pursuer"), 0.0f, 0.0f, 0.0f, 1.8f, TEXT("spawn_pursuer:pursuer_far"), 0.0f),
		    Step(TEXT(""), 80.0f, 0.0f, 0.0f, 0.6f),
		    Step(TEXT(""), -80.0f, 0.0f, 0.0f, 0.6f, TEXT("move_pursuer:pursuer_near"), 0.0f),
		    Step(TEXT(""), 0.0f, 0.0f, 0.0f, 2.5f) } },

		{ TEXT("pursuer2"), TEXT("pursuer, pursuer_far, pursuer_mid, pursuer_near"),
		  TEXT("Same, two glances: far -> mid -> near. Stronger read of 'he only moves when you are not looking'."),
		  { Step(TEXT("pursuer"), 0.0f, 0.0f, 0.0f, 1.8f, TEXT("spawn_pursuer:pursuer_far"), 0.0f),
		    Step(TEXT(""), 80.0f, 0.0f, 0.0f, 0.6f),
		    Step(TEXT(""), -80.0f, 0.0f, 0.0f, 0.6f, TEXT("move_pursuer:pursuer_mid"), 0.0f),
		    Step(TEXT(""), 0.0f, 0.0f, 0.0f, 1.2f),
		    Step(TEXT(""), 80.0f, 0.0f, 0.0f, 0.6f),
		    Step(TEXT(""), -80.0f, 0.0f, 0.0f, 0.6f, TEXT("move_pursuer:pursuer_near"), 0.0f),
		    Step(TEXT(""), 0.0f, 0.0f, 0.0f, 2.5f) } },

		{ TEXT("mag"), TEXT("mag"),
		  TEXT("Tight on the magazine, only the word in frame. Text forced, then a short push in."),
		  { Step(TEXT("mag"), 0.0f, 0.0f, 0.0f, 1.0f, TEXT("Text"), 0.0f),
		    Step(TEXT(""), 0.0f, 0.0f, 40.0f, 2.5f) } },

		{ TEXT("phone_dark"), TEXT("phone"),
		  TEXT("Closing shot. Mark ~3.5 m from a desk phone, facing it. Phone forced without the outage sound, floor drops to the one lamp, a slow walk in looking down, then the pickup and black."),
		  { Step(TEXT("phone"), 0.0f, 0.0f, 0.0f, 1.5f, TEXT("phone_quiet"), 0.0f),
		    Step(TEXT(""), 0.0f, -14.0f, 300.0f, 6.0f),
		    Step(TEXT(""), 0.0f, 0.0f, 0.0f, 2.5f, TEXT("pickup"), 0.0f) } },
	};
	return Scenes;
}

FString ULoop9TrailerRigSubsystem::GetMarksFilePath() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Trailer"), TEXT("Marks.ini"));
}

bool ULoop9TrailerRigSubsystem::MarkHere(const APlayerController* InController, const FString& Name, FString& OutMessage)
{
	const APawn* Pawn = InController ? InController->GetPawn() : nullptr;
	if (!Pawn || Name.IsEmpty())
	{
		OutMessage = TEXT("TrailerMark: no pawn, or no name given.");
		return false;
	}

	const FVector Location = Pawn->GetActorLocation();
	// Control rotation comes back in [0, 360); store it signed so a -5 pitch is
	// not read back as 355 and clamped into the ceiling.
	const FRotator Rotation = InController->GetControlRotation().GetNormalized();

	const FString Path = GetMarksFilePath();
	FConfigFile File;
	File.Read(Path);
	File.SetString(MarksSection, *Name, *FString::Printf(TEXT("%s|%s"), *Location.ToString(), *Rotation.ToString()));
	File.Dirty = true;
	if (!File.Write(Path))
	{
		OutMessage = FString::Printf(TEXT("TrailerMark: could not write %s"), *Path);
		return false;
	}

	OutMessage = FString::Printf(TEXT("TrailerMark '%s' saved at %s yaw %.1f pitch %.1f"),
		*Name, *Location.ToCompactString(), Rotation.Yaw, Rotation.Pitch);
	return true;
}

bool ULoop9TrailerRigSubsystem::LoadMark(const FString& Name, FVector& OutLocation, FRotator& OutRotation) const
{
	FConfigFile File;
	File.Read(GetMarksFilePath());

	FString Value;
	if (!File.GetString(MarksSection, *Name, Value))
	{
		return false;
	}

	FString LocationText;
	FString RotationText;
	if (!Value.Split(TEXT("|"), &LocationText, &RotationText))
	{
		return false;
	}
	if (!OutLocation.InitFromString(LocationText) || !OutRotation.InitFromString(RotationText))
	{
		return false;
	}
	OutRotation = OutRotation.GetNormalized();
	OutRotation.Roll = 0.0f;
	return true;
}

TArray<FString> ULoop9TrailerRigSubsystem::ListMarks() const
{
	TArray<FString> Names;
	FConfigFile File;
	File.Read(GetMarksFilePath());
	if (const FConfigSection* Section = File.FindSection(MarksSection))
	{
		for (const TPair<FName, FConfigValue>& Pair : *Section)
		{
			Names.Add(Pair.Key.ToString());
		}
	}
	Names.Sort();
	return Names;
}

bool ULoop9TrailerRigSubsystem::StartShot(
	APlayerController* InController,
	const FString& MarkName,
	float PanDegrees,
	float PitchDegrees,
	float DollyCm,
	float Seconds,
	FString& OutMessage)
{
	TArray<FTrailerStep> Single;
	Single.Add(Step(*MarkName, PanDegrees, PitchDegrees, DollyCm, FMath::Max(Seconds, 0.1f)));
	const FString ShotLabel = FString::Printf(TEXT("shot %s: pan %.1f pitch %.1f dolly %.0f cm over %.1f s"),
		*MarkName, PanDegrees, PitchDegrees, DollyCm, Seconds);
	return BeginSteps(InController, ShotLabel, MoveTemp(Single), OutMessage);
}

bool ULoop9TrailerRigSubsystem::StartScene(APlayerController* InController, const FString& SceneName, FString& OutMessage)
{
	for (const FTrailerScene& Scene : GetScenes())
	{
		if (Scene.Name.Equals(SceneName, ESearchCase::IgnoreCase))
		{
			TArray<FTrailerStep> Copy = Scene.Steps;
			return BeginSteps(InController, FString::Printf(TEXT("scene %s"), *Scene.Name), MoveTemp(Copy), OutMessage);
		}
	}
	OutMessage = FString::Printf(TEXT("TrailerScene: no scene '%s'. TrailerScenes lists them."), *SceneName);
	return false;
}

bool ULoop9TrailerRigSubsystem::BeginSteps(APlayerController* InController, const FString& InLabel, TArray<FTrailerStep>&& InSteps, FString& OutMessage)
{
	StopShot();

	APawn* Pawn = InController ? InController->GetPawn() : nullptr;
	if (!Pawn || InSteps.Num() == 0)
	{
		OutMessage = TEXT("Trailer rig: no pawn.");
		return false;
	}

	// Fail before moving anything if a mark is missing, so a scene never runs half-way.
	for (const FTrailerStep& S : InSteps)
	{
		FVector Loc;
		FRotator Rot;
		for (const FString& Needed : { S.Mark, MarkFromAction(S.Action) })
		{
			if (!Needed.IsEmpty() && !LoadMark(Needed, Loc, Rot))
			{
				OutMessage = FString::Printf(TEXT("Trailer rig: mark '%s' is not set. Stand there and TrailerMark %s."), *Needed, *Needed);
				return false;
			}
		}
	}

	Controller = InController;
	Steps = MoveTemp(InSteps);
	Label = InLabel;

	InController->SetIgnoreLookInput(true);
	InController->SetIgnoreMoveInput(true);
	bInputIgnored = true;
	bRunning = true;

	// Clean frame and clean audio for the capture: no crosshair or prompts, and
	// no music bed under the phone / the burst. Both come back when it ends.
	if (ALoop9PlayerController* LoopPC = Cast<ALoop9PlayerController>(InController))
	{
		LoopPC->SetGameplayHUDVisible(false);
		bHudHidden = true;
	}
	if (ALoop9GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALoop9GameMode>() : nullptr)
	{
		GameMode->SetMusicSuppressed(true);
		bMusicSuppressed = true;
	}

	if (!BeginStep(0))
	{
		StopShot();
		OutMessage = TEXT("Trailer rig: could not start.");
		return false;
	}

	float Total = 0.0f;
	for (const FTrailerStep& S : Steps)
	{
		Total += S.Seconds + (S.Mark.IsEmpty() ? 0.0f : TeleportSettleSeconds);
	}
	OutMessage = FString::Printf(TEXT("Trailer %s: %d step(s), ~%.1f s"), *Label, Steps.Num(), Total + HoldSeconds);
	UE_LOG(LogLoop9, Log, TEXT("%s"), *OutMessage);
	return true;
}

bool ULoop9TrailerRigSubsystem::BeginStep(int32 Index)
{
	APlayerController* PC = Controller.Get();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn || !Steps.IsValidIndex(Index))
	{
		return false;
	}

	StepIndex = Index;
	StepElapsed = 0.0f;
	bActionFired = false;
	const FTrailerStep& S = Steps[Index];

	if (!S.Mark.IsEmpty())
	{
		FVector Location;
		FRotator Rotation;
		if (!LoadMark(S.Mark, Location, Rotation))
		{
			return false;
		}
		// Kill any momentum before the teleport so the walk bob starts from rest.
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
			{
				Move->StopMovementImmediately();
			}
		}
		Pawn->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(Rotation);
		BaseRotation = Rotation;
		StepSettle = TeleportSettleSeconds;
	}
	else
	{
		// Continue from where the previous step left the camera.
		BaseRotation = PC->GetControlRotation().GetNormalized();
		BaseRotation.Roll = 0.0f;
		StepSettle = 0.0f;
	}
	DollyDirection = FRotator(0.0f, BaseRotation.Yaw, 0.0f).Vector();

	if (!S.Action.IsEmpty() && S.ActionAtAlpha <= 0.0f)
	{
		RunAction(S.Action);
		bActionFired = true;
	}
	return true;
}

FString ULoop9TrailerRigSubsystem::MarkFromAction(const FString& Action)
{
	FString Verb;
	FString Arg;
	if (Action.Split(TEXT(":"), &Verb, &Arg) &&
		(Verb.Equals(TEXT("spawn_pursuer"), ESearchCase::IgnoreCase) || Verb.Equals(TEXT("move_pursuer"), ESearchCase::IgnoreCase)))
	{
		return Arg.TrimStartAndEnd();
	}
	return FString();
}

void ULoop9TrailerRigSubsystem::RunAction(const FString& Action)
{
	FString Verb = Action;
	FString Arg;
	Action.Split(TEXT(":"), &Verb, &Arg);
	Verb.TrimStartAndEndInline();
	Arg.TrimStartAndEndInline();

	if (Verb.Equals(TEXT("spawn_pursuer"), ESearchCase::IgnoreCase))
	{
		SpawnRigPursuer(Arg);
	}
	else if (Verb.Equals(TEXT("move_pursuer"), ESearchCase::IgnoreCase))
	{
		MoveRigPursuer(Arg);
	}
	else if (Verb.Equals(TEXT("phone_quiet"), ESearchCase::IgnoreCase))
	{
		if (ULoop9RingingFloorSubsystem* Ringing = GetWorld() ? GetWorld()->GetSubsystem<ULoop9RingingFloorSubsystem>() : nullptr)
		{
			Ringing->SetBlackoutSoundMuted(true);
		}
		FireForce(TEXT("Phone"));
	}
	else if (Verb.Equals(TEXT("pickup"), ESearchCase::IgnoreCase))
	{
		PhonePickup();
	}
	else if (Verb.Equals(TEXT("fade_out"), ESearchCase::IgnoreCase))
	{
		FadeOut();
	}
	else
	{
		FireForce(Action);
	}
}

bool ULoop9TrailerRigSubsystem::PursuerSpotFromMark(const FString& MarkName, FVector& OutLocation, FRotator& OutRotation) const
{
	const APlayerController* PC = Controller.Get();
	const ACharacter* Player = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	FRotator Unused;
	if (!Player || !LoadMark(MarkName, OutLocation, Unused))
	{
		return false;
	}

	// Marks store the player's capsule centre; put the Pursuer's centre at the
	// same floor height. His capsule can be a different size.
	float PursuerHalfHeight = Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	if (const APursuerAnomalyCharacter* Existing = RigPursuer.Get())
	{
		PursuerHalfHeight = Existing->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	OutLocation.Z += PursuerHalfHeight - Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// Never inside the catch radius: the character would count it as a catch and despawn.
	const FVector PlayerLocation = Player->GetActorLocation();
	FVector ToSpot = OutLocation - PlayerLocation;
	ToSpot.Z = 0.0f;
	if (ToSpot.SizeSquared() < FMath::Square(PursuerMinDistanceCm))
	{
		const FVector Dir = ToSpot.IsNearlyZero() ? FRotator(0.0f, PC->GetControlRotation().Yaw, 0.0f).Vector() : ToSpot.GetSafeNormal();
		OutLocation = FVector(PlayerLocation.X, PlayerLocation.Y, OutLocation.Z) + Dir * PursuerMinDistanceCm;
	}

	// Face the player, upright.
	OutRotation = FRotator(0.0f, (PlayerLocation - OutLocation).Rotation().Yaw, 0.0f);
	return true;
}

void ULoop9TrailerRigSubsystem::SpawnRigPursuer(const FString& MarkName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (APursuerAnomalyCharacter* Old = RigPursuer.Get())
	{
		Old->Destroy();
		RigPursuer.Reset();
	}

	// The class is only known to the anomaly component placed on the floor.
	TSubclassOf<APursuerAnomalyCharacter> PursuerClass;
	for (TObjectIterator<UPursuerAnomalyComponent> It; It; ++It)
	{
		if (It->GetWorld() == World && It->PursuerClass)
		{
			PursuerClass = It->PursuerClass;
			break;
		}
	}
	if (!PursuerClass)
	{
		UE_LOG(LogLoop9, Warning, TEXT("Trailer %s: no Pursuer anomaly component with a PursuerClass on this floor; nothing spawned."), *Label);
		return;
	}

	FVector Location;
	FRotator Rotation;
	if (!PursuerSpotFromMark(MarkName, Location, Rotation))
	{
		return;
	}
	// A CDO capsule is what we have before he exists; correct the height with it.
	if (const ACharacter* CDO = PursuerClass->GetDefaultObject<ACharacter>())
	{
		if (const ACharacter* Player = Cast<ACharacter>(Controller->GetPawn()))
		{
			Location.Z += CDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	APursuerAnomalyCharacter* Pursuer = World->SpawnActor<APursuerAnomalyCharacter>(PursuerClass, Location, Rotation, Params);
	if (!Pursuer)
	{
		UE_LOG(LogLoop9, Warning, TEXT("Trailer %s: Pursuer spawn failed at %s."), *Label, *Location.ToCompactString());
		return;
	}

	// A prop, not a hunter: he stands where the rig puts him and stays for the shot.
	Pursuer->MaxLifetime = 600.0f;
	if (UCharacterMovementComponent* Move = Pursuer->GetCharacterMovement())
	{
		Move->MaxWalkSpeed = 0.0f;
		Move->StopMovementImmediately();
	}
	RigPursuer = Pursuer;

	if (UGameInstance* GI = World->GetGameInstance())
	{
		if (ULoop9TelemetrySubsystem* Telemetry = GI->GetSubsystem<ULoop9TelemetrySubsystem>())
		{
			Telemetry->MarkRunTaintedByDebug();
		}
	}
	UE_LOG(LogLoop9, Log, TEXT("Trailer %s: Pursuer placed on '%s' at %s."), *Label, *MarkName, *Location.ToCompactString());
}

void ULoop9TrailerRigSubsystem::MoveRigPursuer(const FString& MarkName)
{
	APursuerAnomalyCharacter* Pursuer = RigPursuer.Get();
	if (!Pursuer)
	{
		UE_LOG(LogLoop9, Warning, TEXT("Trailer %s: move_pursuer before spawn_pursuer; nothing to move."), *Label);
		return;
	}
	FVector Location;
	FRotator Rotation;
	if (!PursuerSpotFromMark(MarkName, Location, Rotation))
	{
		return;
	}
	if (UCharacterMovementComponent* Move = Pursuer->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	Pursuer->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	UE_LOG(LogLoop9, Log, TEXT("Trailer %s: Pursuer moved to '%s'."), *Label, *MarkName);
}

void ULoop9TrailerRigSubsystem::PhonePickup()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// Silence every ringing desk phone the way answering does, keep the anomaly.
	for (TObjectIterator<UAudioAnomalyComponent> It; It; ++It)
	{
		if (It->GetWorld() == World && It->IsRinging())
		{
			It->Answer();
		}
	}
	if (USoundBase* Pickup = LoadObject<USoundBase>(nullptr, PickupSoundPath))
	{
		if (UAudioComponent* Audio = UGameplayStatics::SpawnSound2D(World, Pickup))
		{
			Audio->SetUISound(false);
		}
	}
	FadeOut();
}

void ULoop9TrailerRigSubsystem::FadeOut()
{
	APlayerController* PC = Controller.Get();
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, PickupFadeSeconds, FLinearColor::Black, false, true);
		bFadedOut = true;
	}
}

void ULoop9TrailerRigSubsystem::FireForce(const FString& Filter)
{
#if !UE_BUILD_SHIPPING
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (!GI)
	{
		return;
	}
	if (ULoop9TelemetrySubsystem* Telemetry = GI->GetSubsystem<ULoop9TelemetrySubsystem>())
	{
		Telemetry->MarkRunTaintedByDebug();
	}
	if (UAnomalyManager* Manager = GI->GetSubsystem<UAnomalyManager>())
	{
		const bool bOk = Manager->ForceActivateByFilter(Filter, INDEX_NONE);
		UE_LOG(LogLoop9, Log, TEXT("Trailer %s: AnomalyForce %s -> %s"), *Label, *Filter, bOk ? TEXT("ok") : TEXT("nothing matched"));
	}
#else
	(void)Filter;
#endif
}

void ULoop9TrailerRigSubsystem::StopShot()
{
	if (bInputIgnored)
	{
		if (APlayerController* PC = Controller.Get())
		{
			PC->SetIgnoreLookInput(false);
			PC->SetIgnoreMoveInput(false);
		}
		bInputIgnored = false;
	}
	if (bHudHidden)
	{
		if (ALoop9PlayerController* LoopPC = Cast<ALoop9PlayerController>(Controller.Get()))
		{
			LoopPC->SetGameplayHUDVisible(true);
		}
		bHudHidden = false;
	}
	if (bMusicSuppressed)
	{
		if (ALoop9GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALoop9GameMode>() : nullptr)
		{
			GameMode->SetMusicSuppressed(false);
		}
		bMusicSuppressed = false;
	}
	if (bFadedOut)
	{
		if (APlayerController* PC = Controller.Get())
		{
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeBackSeconds, FLinearColor::Black, false, false);
			}
		}
		bFadedOut = false;
	}
	if (APursuerAnomalyCharacter* Pursuer = RigPursuer.Get())
	{
		Pursuer->Destroy();
	}
	RigPursuer.Reset();
	if (ULoop9RingingFloorSubsystem* Ringing = GetWorld() ? GetWorld()->GetSubsystem<ULoop9RingingFloorSubsystem>() : nullptr)
	{
		Ringing->SetBlackoutSoundMuted(false);
	}
	if (bRunning)
	{
		UE_LOG(LogLoop9, Log, TEXT("Trailer %s: done"), *Label);
	}
	bRunning = false;
	StepIndex = INDEX_NONE;
	Steps.Reset();
	Controller.Reset();
}

float ULoop9TrailerRigSubsystem::EaseInOut(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T * (3.0f - 2.0f * T);
}

void ULoop9TrailerRigSubsystem::Tick(float DeltaTime)
{
	APlayerController* PC = Controller.Get();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn || !Steps.IsValidIndex(StepIndex))
	{
		StopShot();
		return;
	}

	const FTrailerStep& S = Steps[StepIndex];
	StepElapsed += DeltaTime;

	if (StepElapsed < StepSettle)
	{
		PC->SetControlRotation(BaseRotation);
		return;
	}

	const float MoveTime = StepElapsed - StepSettle;
	const float Alpha = EaseInOut(MoveTime / S.Seconds);

	if (!bActionFired && !S.Action.IsEmpty() && Alpha >= S.ActionAtAlpha)
	{
		bActionFired = true;
		RunAction(S.Action);
	}

	FRotator Rotation = BaseRotation;
	Rotation.Yaw += S.PanDegrees * Alpha;
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch + S.PitchDegrees * Alpha, -85.0f, 85.0f);
	PC->SetControlRotation(Rotation);

	if (!FMath::IsNearlyZero(S.DollyCm) && MoveTime < S.Seconds)
	{
		float MaxSpeed = 300.0f;
		if (const ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (const UCharacterMovementComponent* Move = Character->GetCharacterMovement())
			{
				MaxSpeed = FMath::Max(Move->MaxWalkSpeed, 1.0f);
			}
		}
		// Analog input scales the walk speed, so a slow dolly still carries the
		// walk bob instead of snapping between standing and full pace. bForce:
		// move input is ignored for the player during the shot, and that gate
		// would otherwise swallow the rig's own input too.
		const float Speed = FMath::Abs(S.DollyCm) / S.Seconds;
		const float Scale = FMath::Clamp(Speed / MaxSpeed, 0.0f, 1.0f) * FMath::Sign(S.DollyCm);
		Pawn->AddMovementInput(DollyDirection, Scale, /*bForce*/ true);
	}

	if (MoveTime < S.Seconds)
	{
		return;
	}

	const bool bLastStep = StepIndex == Steps.Num() - 1;
	if (bLastStep)
	{
		if (MoveTime >= S.Seconds + HoldSeconds)
		{
			StopShot();
		}
		return;
	}

	if (!BeginStep(StepIndex + 1))
	{
		StopShot();
	}
}
