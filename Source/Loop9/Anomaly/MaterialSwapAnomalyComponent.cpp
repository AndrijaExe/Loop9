#include "Anomaly/MaterialSwapAnomalyComponent.h"

#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

UMaterialSwapAnomalyComponent::UMaterialSwapAnomalyComponent()
{
	AnomalyProbability = 0.5f;
}

UMeshComponent* UMaterialSwapAnomalyComponent::ResolveTargetMesh() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (!TargetComponentName.IsNone())
	{
		TArray<UMeshComponent*> MeshComponents;
		Owner->GetComponents<UMeshComponent>(MeshComponents);
		for (UMeshComponent* Mesh : MeshComponents)
		{
			if (Mesh && Mesh->GetFName() == TargetComponentName)
			{
				return Mesh;
			}
		}
		return nullptr;
	}

	return Owner->FindComponentByClass<UMeshComponent>();
}

void UMaterialSwapAnomalyComponent::BeginPlay()
{
	TargetMesh = ResolveTargetMesh();
	if (TargetMesh && TargetMesh->GetNumMaterials() > MaterialSlot)
	{
		NormalMaterial = TargetMesh->GetMaterial(MaterialSlot);
		bNormalMaterialCaptured = true;
	}

	Super::BeginPlay();
}

bool UMaterialSwapAnomalyComponent::ApplyAnomalyState()
{
	if (!TargetMesh || AnomalyMaterials.Num() == 0)
	{
		return false;
	}

	// Remember the normal material lazily too, in case the slot changed
	// after BeginPlay (e.g. some other system set a material).
	if (!bNormalMaterialCaptured && TargetMesh->GetNumMaterials() > MaterialSlot)
	{
		NormalMaterial = TargetMesh->GetMaterial(MaterialSlot);
		bNormalMaterialCaptured = true;
	}

	UMaterialInterface* Chosen = AnomalyMaterials[FMath::RandRange(0, AnomalyMaterials.Num() - 1)];
	if (!Chosen)
	{
		return false;
	}

	TargetMesh->SetMaterial(MaterialSlot, Chosen);
	return true;
}

void UMaterialSwapAnomalyComponent::RestoreNormalState()
{
	if (TargetMesh && bNormalMaterialCaptured)
	{
		TargetMesh->SetMaterial(MaterialSlot, NormalMaterial);
	}
}
