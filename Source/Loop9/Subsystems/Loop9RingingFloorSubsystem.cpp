#include "Subsystems/Loop9RingingFloorSubsystem.h"

#include "AI_Friend.h"
#include "Components/LightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Interaction/LoopElevatorTransitionDirector.h"
#include "Kismet/GameplayStatics.h"
#include "LiftDoorWing.h"
#include "Loop9.h"
#include "Subsystems/Loop9LightsSubsystem.h"
#include "TeleportPoint.h"
#include "TimerManager.h"

namespace
{
	constexpr float ExitPollIntervalSeconds = 0.2f;
	/** Without a director or an arrival point there is no lift to watch; hold after this long instead. */
	constexpr double ExitPollFallbackSeconds = 4.0;
}

void ULoop9RingingFloorSubsystem::Deinitialize()
{
	ClearState();
	Super::Deinitialize();
}

ALoopElevatorTransitionDirector* ULoop9RingingFloorSubsystem::FindDirector() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ALoopElevatorTransitionDirector> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				return *It;
			}
		}
	}
	return nullptr;
}

void ULoop9RingingFloorSubsystem::BeginRingingFloor(AAI_Friend* Phone)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Phone))
	{
		return;
	}

	if (RingingPhone.IsValid() && RingingPhone.Get() != Phone)
	{
		// Two ringing phones on one floor: the first one owns the beat.
		return;
	}

	ClearState();
	RingingPhone = Phone;
	Director = FindDirector();
	ExitPollStartedAtSeconds = World->GetTimeSeconds();

	World->GetTimerManager().SetTimer(
		ExitPollTimerHandle, this, &ULoop9RingingFloorSubsystem::PollForLiftExit, ExitPollIntervalSeconds, true);

	UE_LOG(LogLoop9, Log, TEXT("Ringing floor: '%s' is ringing, waiting for the player to leave the lift."),
		*Phone->GetActorNameOrLabel());
}

void ULoop9RingingFloorSubsystem::PollForLiftExit()
{
	UWorld* World = GetWorld();
	if (!World || !RingingPhone.IsValid())
	{
		ClearState();
		return;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn)
	{
		return;
	}

	const ALoopElevatorTransitionDirector* Dir = Director.Get();
	if (Dir && Dir->IsTransitionInProgress())
	{
		// Doors still opening; the player has not had the chance to leave.
		return;
	}

	const ATeleportPoint* Arrival = Dir ? Dir->LitElevatorArrivalPoint.Get() : nullptr;
	if (Arrival)
	{
		const float Distance = static_cast<float>(FVector::Dist2D(PlayerPawn->GetActorLocation(), Arrival->GetActorLocation()));
		if (Distance < LiftExitDistanceCm)
		{
			return;
		}
	}
	else if (World->GetTimeSeconds() - ExitPollStartedAtSeconds < ExitPollFallbackSeconds)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(ExitPollTimerHandle);
	HoldLiftAndBlackout();
}

void ULoop9RingingFloorSubsystem::HoldLiftAndBlackout()
{
	UWorld* World = GetWorld();
	AAI_Friend* Phone = RingingPhone.Get();
	if (!World || !Phone)
	{
		ClearState();
		return;
	}

	if (ALoopElevatorTransitionDirector* Dir = Director.Get())
	{
		for (ALiftDoorWing* Wing : Dir->ArrivalDoorWings)
		{
			if (IsValid(Wing))
			{
				Wing->CloseDoorWing();
			}
		}
	}
	bLitLiftHeld = true;

	if (ULoop9LightsSubsystem* Lights = World->GetSubsystem<ULoop9LightsSubsystem>())
	{
		TArray<ULightComponent*> KeepLit;
		if (ULightComponent* Lamp = Lights->FindNearestLight(Phone->GetActorLocation(), PhoneLampSearchRadiusCm))
		{
			KeepLit.Add(Lamp);
		}
		else
		{
			UE_LOG(LogLoop9, Warning, TEXT("Ringing floor: no lamp within %.0f cm of '%s'; the floor goes fully dark."),
				PhoneLampSearchRadiusCm, *Phone->GetActorNameOrLabel());
		}
		Lights->Blackout(KeepLit);
		bBlackoutApplied = true;
	}

	if (MaxHoldSeconds > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			MaxHoldTimerHandle, this, &ULoop9RingingFloorSubsystem::ReleaseLiftAndRestore, MaxHoldSeconds, false);
	}

	UE_LOG(LogLoop9, Log, TEXT("Ringing floor: lit lift closed, lights out except above the phone."));
}

void ULoop9RingingFloorSubsystem::OnRingingMessageRead(const AAI_Friend* Phone)
{
	if (!RingingPhone.IsValid() || RingingPhone.Get() != Phone)
	{
		return;
	}
	ReleaseLiftAndRestore();
}

void ULoop9RingingFloorSubsystem::EndRingingFloor(const AAI_Friend* Phone)
{
	if (!RingingPhone.IsValid() || RingingPhone.Get() != Phone)
	{
		return;
	}
	ReleaseLiftAndRestore();
	ClearState();
}

void ULoop9RingingFloorSubsystem::ReleaseLiftAndRestore()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(ExitPollTimerHandle);
		World->GetTimerManager().ClearTimer(MaxHoldTimerHandle);
	}

	if (bBlackoutApplied && World)
	{
		if (ULoop9LightsSubsystem* Lights = World->GetSubsystem<ULoop9LightsSubsystem>())
		{
			Lights->Restore();
		}
		bBlackoutApplied = false;
	}

	if (bLitLiftHeld)
	{
		if (ALoopElevatorTransitionDirector* Dir = Director.Get())
		{
			if (!Dir->IsTransitionInProgress())
			{
				for (ALiftDoorWing* Wing : Dir->ArrivalDoorWings)
				{
					if (IsValid(Wing))
					{
						Wing->OpenDoorWing();
					}
				}
			}
		}
		bLitLiftHeld = false;
		UE_LOG(LogLoop9, Log, TEXT("Ringing floor: lights back, lit lift released."));
	}
}

void ULoop9RingingFloorSubsystem::ClearState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExitPollTimerHandle);
		World->GetTimerManager().ClearTimer(MaxHoldTimerHandle);
	}
	RingingPhone.Reset();
	Director.Reset();
	bLitLiftHeld = false;
	bBlackoutApplied = false;
}
