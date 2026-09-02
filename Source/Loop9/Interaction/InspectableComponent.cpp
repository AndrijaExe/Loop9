#include "Interaction/InspectableComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/InspectionStageActor.h"
#include "Loop9.h"
#include "Subsystems/Loop9ObservationJournalSubsystem.h"

UInspectableComponent::UInspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UStaticMeshComponent* UInspectableComponent::ResolveTargetMeshComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TArray<UStaticMeshComponent*> MeshComponents;
	Owner->GetComponents<UStaticMeshComponent>(MeshComponents);

	if (!TargetMeshComponentName.IsNone())
	{
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			if (MeshComponent && MeshComponent->GetFName() == TargetMeshComponentName)
			{
				return MeshComponent;
			}
		}

		UE_LOG(LogLoop9, Warning,
			TEXT("InspectableComponent on '%s': target mesh component '%s' was not found."),
			*GetNameSafe(Owner), *TargetMeshComponentName.ToString());
		return nullptr;
	}

	return MeshComponents.Num() > 0 ? MeshComponents[0] : nullptr;
}

UStaticMesh* UInspectableComponent::ResolveMesh(TArray<UMaterialInterface*>& OutMaterialOverrides) const
{
	OutMaterialOverrides.Reset();

	UStaticMeshComponent* MeshComponent = ResolveTargetMeshComponent();
	UStaticMesh* ResolvedMesh = MeshOverride;
	if (!ResolvedMesh && MeshComponent)
	{
		ResolvedMesh = MeshComponent->GetStaticMesh();
	}

	if (!ResolvedMesh)
	{
		return nullptr;
	}

	if (MeshComponent && (!MeshOverride || bCopyOwnerMaterialsWithMeshOverride))
	{
		// Preserve live per-instance changes, including material anomalies.
		OutMaterialOverrides = MeshComponent->GetMaterials();
	}

	if (InspectionMaterialOverrides.Num() > 0)
	{
		OutMaterialOverrides.SetNum(FMath::Max(
			OutMaterialOverrides.Num(), InspectionMaterialOverrides.Num()));
		for (int32 Index = 0; Index < InspectionMaterialOverrides.Num(); ++Index)
		{
			if (InspectionMaterialOverrides[Index])
			{
				OutMaterialOverrides[Index] = InspectionMaterialOverrides[Index];
			}
		}
	}

	return ResolvedMesh;
}

bool UInspectableComponent::StartInspection(APlayerController* InteractingController)
{
	if (!InteractingController)
	{
		return false;
	}

	if (AInspectionStageActor::IsInspectionActive())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AInspectionStageActor* Stage = World->SpawnActor<AInspectionStageActor>(SpawnParams);
	if (!Stage)
	{
		return false;
	}

	if (!Stage->BeginInspection(this, InteractingController))
	{
		Stage->Destroy();
		return false;
	}

	UE_LOG(LogLoop9, Log, TEXT("Inspection started for '%s'"), *GetNameSafe(GetOwner()));
	OnInspectionStarted.Broadcast();
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (ULoop9ObservationJournalSubsystem* Journal =
			GameInstance->GetSubsystem<ULoop9ObservationJournalSubsystem>())
		{
			Journal->RecordObjectInspected(ObservationId);
		}
	}
	return true;
}

void UInspectableComponent::NotifyInspectionEnded()
{
	UE_LOG(LogLoop9, Log, TEXT("Inspection ended for '%s'"), *GetNameSafe(GetOwner()));
	OnInspectionEnded.Broadcast();
}
