#include "TextSpawnAnomalyComponent.h"
#include "Subsystems/AnomalyManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"

UTextSpawnAnomalyComponent::UTextSpawnAnomalyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTextSpawnAnomalyComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		if (UAnomalyManager* Manager = GI->GetSubsystem<UAnomalyManager>())
		{
			Manager->RegisterAnomaly(GetOwner());
		}
	}
}

void UTextSpawnAnomalyComponent::ActivateAnomaly()
{
	if (bIsAnomalyActive)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	bIsAnomalyActive = true;

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
		return;
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
				const FRotator FaceRot = ToPlayer.Rotation();
				SpawnedTextComponent->SetWorldRotation(FaceRot);
			}
		}
	}
}

void UTextSpawnAnomalyComponent::DeactivateAnomaly()
{
	if (!bIsAnomalyActive)
	{
		return;
	}

	bIsAnomalyActive = false;

	if (SpawnedTextComponent)
	{
		SpawnedTextComponent->DestroyComponent();
		SpawnedTextComponent = nullptr;
	}
}
