#include "Subsystems/Loop9TrailerRigSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Loop9.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

namespace
{
	/** Lets the camera lag catch up on the teleport before the move starts. */
	constexpr float SettleSeconds = 0.4f;
	/** Keeps the frame still after the move so the cut has a clean tail. */
	constexpr float HoldSeconds = 0.6f;
	const TCHAR* MarksSection = TEXT("Marks");
}

FString ULoop9TrailerRigSubsystem::GetMarksFilePath() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Trailer"), TEXT("Marks.ini"));
}

bool ULoop9TrailerRigSubsystem::MarkHere(const APlayerController* Controller, const FString& Name, FString& OutMessage)
{
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn || Name.IsEmpty())
	{
		OutMessage = TEXT("TrailerMark: no pawn, or no name given.");
		return false;
	}

	const FVector Location = Pawn->GetActorLocation();
	// Control rotation comes back in [0, 360); store it signed so a -5 pitch is
	// not read back as 355 and clamped into the ceiling.
	const FRotator Rotation = Controller->GetControlRotation().GetNormalized();

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
	APlayerController* Controller,
	const FString& Name,
	float InPanDegrees,
	float InPitchDegrees,
	float InDollyCm,
	float Seconds,
	FString& OutMessage)
{
	StopShot();

	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		OutMessage = TEXT("TrailerShot: no pawn.");
		return false;
	}

	FVector Location;
	FRotator Rotation;
	if (!LoadMark(Name, Location, Rotation))
	{
		OutMessage = FString::Printf(TEXT("TrailerShot: no mark '%s'. TrailerList shows what exists."), *Name);
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
	Controller->SetControlRotation(Rotation);

	Controller->SetIgnoreLookInput(true);
	Controller->SetIgnoreMoveInput(true);
	bInputIgnored = true;

	ShotController = Controller;
	ShotName = Name;
	BaseRotation = Rotation;
	DollyDirection = FRotator(0.0f, Rotation.Yaw, 0.0f).Vector();
	PanDegrees = InPanDegrees;
	PitchDegrees = InPitchDegrees;
	DollyCm = InDollyCm;
	MoveSeconds = FMath::Max(Seconds, 0.1f);
	Elapsed = 0.0f;
	bShotRunning = true;

	OutMessage = FString::Printf(TEXT("TrailerShot '%s': pan %.1f pitch %.1f dolly %.0f cm over %.1f s (+%.1f s settle, +%.1f s hold)"),
		*Name, PanDegrees, PitchDegrees, DollyCm, MoveSeconds, SettleSeconds, HoldSeconds);
	UE_LOG(LogLoop9, Log, TEXT("%s"), *OutMessage);
	return true;
}

void ULoop9TrailerRigSubsystem::StopShot()
{
	if (bInputIgnored)
	{
		if (APlayerController* Controller = ShotController.Get())
		{
			Controller->SetIgnoreLookInput(false);
			Controller->SetIgnoreMoveInput(false);
		}
		bInputIgnored = false;
	}
	if (bShotRunning)
	{
		UE_LOG(LogLoop9, Log, TEXT("TrailerShot '%s': done"), *ShotName);
	}
	bShotRunning = false;
	ShotController.Reset();
}

float ULoop9TrailerRigSubsystem::EaseInOut(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T * (3.0f - 2.0f * T);
}

void ULoop9TrailerRigSubsystem::Tick(float DeltaTime)
{
	APlayerController* Controller = ShotController.Get();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		StopShot();
		return;
	}

	Elapsed += DeltaTime;

	if (Elapsed < SettleSeconds)
	{
		Controller->SetControlRotation(BaseRotation);
		return;
	}

	const float MoveTime = Elapsed - SettleSeconds;
	const float Alpha = EaseInOut(MoveTime / MoveSeconds);

	FRotator Rotation = BaseRotation;
	Rotation.Yaw += PanDegrees * Alpha;
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch + PitchDegrees * Alpha, -85.0f, 85.0f);
	Controller->SetControlRotation(Rotation);

	if (!FMath::IsNearlyZero(DollyCm) && MoveTime < MoveSeconds)
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
		// walk bob instead of snapping between standing and full pace.
		const float Speed = FMath::Abs(DollyCm) / MoveSeconds;
		const float Scale = FMath::Clamp(Speed / MaxSpeed, 0.0f, 1.0f) * FMath::Sign(DollyCm);
		// bForce: move input is ignored for the player during the shot, and that
		// gate would otherwise swallow the rig's own input too.
		Pawn->AddMovementInput(DollyDirection, Scale, /*bForce*/ true);
	}

	if (MoveTime >= MoveSeconds + HoldSeconds)
	{
		StopShot();
	}
}
