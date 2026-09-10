#include "Subsystems/Loop9LightsSubsystem.h"

#include "Components/LightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Interaction/LiftButton.h"
#include "Loop9.h"
#include "TimerManager.h"

void ULoop9LightsSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RestoreTimerHandle);
	}
	SavedLights.Reset();
	Super::Deinitialize();
}

bool ULoop9LightsSubsystem::IsWorldLight(const ULightComponent* Light)
{
	const AActor* Owner = Light ? Light->GetOwner() : nullptr;
	if (!Owner)
	{
		return false;
	}
	// The flashlight belongs to the player; the indicator belongs to the lift.
	if (Owner->IsA<APawn>() || Owner->IsA<ALiftButton>())
	{
		return false;
	}
	return true;
}

void ULoop9LightsSubsystem::Blackout(const TArray<ULightComponent*>& KeepLit)
{
	Restore();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		TArray<ULightComponent*> Lights;
		Actor->GetComponents<ULightComponent>(Lights);
		for (ULightComponent* Light : Lights)
		{
			if (!Light || !IsWorldLight(Light) || KeepLit.Contains(Light))
			{
				continue;
			}

			FSavedLight Saved;
			Saved.Light = Light;
			Saved.Intensity = Light->Intensity;
			Saved.bWasVisible = Light->IsVisible();
			SavedLights.Add(Saved);

			Light->SetIntensity(0.0f);
			Light->SetVisibility(false);
			++Count;
		}
	}

	UE_LOG(LogLoop9, Log, TEXT("Lights: blackout, %d lights off, %d kept lit"), Count, KeepLit.Num());
}

void ULoop9LightsSubsystem::BlackoutForSeconds(float Seconds, const TArray<ULightComponent*>& KeepLit)
{
	Blackout(KeepLit);

	UWorld* World = GetWorld();
	if (!World || Seconds <= 0.0f)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		RestoreTimerHandle, this, &ULoop9LightsSubsystem::Restore, Seconds, false);
}

void ULoop9LightsSubsystem::Restore()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RestoreTimerHandle);
	}

	if (SavedLights.IsEmpty())
	{
		return;
	}

	for (const FSavedLight& Saved : SavedLights)
	{
		if (ULightComponent* Light = Saved.Light.Get())
		{
			Light->SetIntensity(Saved.Intensity);
			Light->SetVisibility(Saved.bWasVisible);
		}
	}
	UE_LOG(LogLoop9, Log, TEXT("Lights: restored %d lights"), SavedLights.Num());
	SavedLights.Reset();
}

ULightComponent* ULoop9LightsSubsystem::FindNearestLight(const FVector& Location, float MaxDistance) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ULightComponent* Best = nullptr;
	float BestDistSq = FMath::Square(FMath::Max(MaxDistance, 0.0f));

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		const AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		TArray<ULightComponent*> Lights;
		Actor->GetComponents<ULightComponent>(Lights);
		for (ULightComponent* Light : Lights)
		{
			if (!Light || !IsWorldLight(Light) || !Light->IsVisible() || Light->Intensity <= 0.0f)
			{
				continue;
			}

			const float DistSq = static_cast<float>(FVector::DistSquared(Light->GetComponentLocation(), Location));
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Light;
			}
		}
	}

	return Best;
}
