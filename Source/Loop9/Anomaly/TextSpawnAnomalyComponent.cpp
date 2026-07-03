#include "Anomaly/TextSpawnAnomalyComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"

UTextSpawnAnomalyComponent::UTextSpawnAnomalyComponent()
{
	AnomalyProbability = 0.5f;
}

bool UTextSpawnAnomalyComponent::ApplyAnomalyState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	if (!SpawnedTextComponent)
	{
		SpawnedTextComponent = NewObject<UTextRenderComponent>(Owner, UTextRenderComponent::StaticClass(), NAME_None, RF_Transient);
		if (SpawnedTextComponent)
		{
			SpawnedTextComponent->RegisterComponent();
			if (USceneComponent* RootComp = Owner->GetRootComponent())
			{
				SpawnedTextComponent->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
			}
		}
	}

	if (!SpawnedTextComponent)
	{
		return false;
	}

	SpawnedTextComponent->SetHorizontalAlignment(EHTA_Center);
	SpawnedTextComponent->SetVerticalAlignment(EVRTA_TextCenter);
	SpawnedTextComponent->SetWorldSize(WorldSize);
	SpawnedTextComponent->SetTextRenderColor(TextColor.ToFColor(true));
	SpawnedTextComponent->SetText(FText::FromString(MessageText));
	SpawnedTextComponent->SetRelativeLocation(RelativeOffset);
	SpawnedTextComponent->SetRelativeRotation(RelativeRotation);
	SpawnedTextComponent->SetHiddenInGame(false);
	SpawnedTextComponent->SetVisibility(true, true);

	if (bFacePlayerOnActivate)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				const FVector ToPlayer = (Pawn->GetActorLocation() - SpawnedTextComponent->GetComponentLocation()).GetSafeNormal();
				SpawnedTextComponent->SetWorldRotation(ToPlayer.Rotation());
			}
		}
	}

	return true;
}

void UTextSpawnAnomalyComponent::RestoreNormalState()
{
	if (SpawnedTextComponent)
	{
		SpawnedTextComponent->DestroyComponent();
		SpawnedTextComponent = nullptr;
	}
}
