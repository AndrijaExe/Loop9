#include "Anomaly/MaterialSwapAnomalyComponent.h"

#include "Loop9.h"
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
	if (TargetMesh && MaterialSlot >= 0 && TargetMesh->GetNumMaterials() > MaterialSlot)
	{
		NormalMaterial = TargetMesh->GetMaterial(MaterialSlot);
		bNormalMaterialCaptured = true;
	}

	Super::BeginPlay();
}

bool UMaterialSwapAnomalyComponent::ForceMaterialVariant(int32 Index)
{
	if (!AnomalyMaterials.IsValidIndex(Index) || !IsValid(AnomalyMaterials[Index]))
	{
		UE_LOG(LogLoop9, Warning, TEXT("MaterialSwapAnomaly on '%s': material index %d is invalid (variants=%d)"),
			GetOwner() ? *GetOwner()->GetActorNameOrLabel() : TEXT("None"),
			Index,
			AnomalyMaterials.Num());
		return false;
	}

	ForcedMaterialIndex = Index;
	if (bIsAnomalyActive)
	{
		const bool bApplied = ApplyAnomalyState();
		ForcedMaterialIndex = INDEX_NONE;
		return bApplied;
	}

	ActivateAnomaly();
	ForcedMaterialIndex = INDEX_NONE;
	return bIsAnomalyActive;
}

bool UMaterialSwapAnomalyComponent::ApplyAnomalyState()
{
	UMeshComponent* ResolvedMesh = ResolveTargetMesh();
	if (TargetMesh != ResolvedMesh)
	{
		TargetMesh = ResolvedMesh;
		NormalMaterial = nullptr;
		bNormalMaterialCaptured = false;
	}

	if (!TargetMesh || AnomalyMaterials.IsEmpty())
	{
		UE_LOG(LogLoop9, Warning, TEXT("MaterialSwapAnomaly on '%s': Apply failed (mesh=%s, mats=%d)"),
			GetOwner() ? *GetOwner()->GetActorNameOrLabel() : TEXT("None"),
			TargetMesh ? TEXT("ok") : TEXT("null"),
			AnomalyMaterials.Num());
		return false;
	}

	if (MaterialSlot < 0 || MaterialSlot >= TargetMesh->GetNumMaterials())
	{
		UE_LOG(LogLoop9, Warning, TEXT("MaterialSwapAnomaly on '%s': invalid material slot %d (slots=%d)"),
			GetOwner() ? *GetOwner()->GetActorNameOrLabel() : TEXT("None"),
			MaterialSlot,
			TargetMesh->GetNumMaterials());
		ForcedMaterialIndex = INDEX_NONE;
		return false;
	}

	// Remember the normal material lazily too, in case the slot changed
	// after BeginPlay (e.g. some other system set a material).
	if (!bNormalMaterialCaptured)
	{
		NormalMaterial = TargetMesh->GetMaterial(MaterialSlot);
		bNormalMaterialCaptured = true;
	}

	int32 ChosenIndex = INDEX_NONE;
	if (ForcedMaterialIndex != INDEX_NONE)
	{
		if (!AnomalyMaterials.IsValidIndex(ForcedMaterialIndex)
			|| !IsValid(AnomalyMaterials[ForcedMaterialIndex]))
		{
			UE_LOG(LogLoop9, Warning, TEXT("MaterialSwapAnomaly on '%s': material index %d is invalid (variants=%d)"),
				GetOwner() ? *GetOwner()->GetActorNameOrLabel() : TEXT("None"),
				ForcedMaterialIndex,
				AnomalyMaterials.Num());
			ForcedMaterialIndex = INDEX_NONE;
			return false;
		}

		ChosenIndex = ForcedMaterialIndex;
		ForcedMaterialIndex = INDEX_NONE;
	}
	else
	{
		int32 ValidMaterialCount = 0;
		for (int32 Index = 0; Index < AnomalyMaterials.Num(); ++Index)
		{
			if (IsValid(AnomalyMaterials[Index]))
			{
				++ValidMaterialCount;
				if (FMath::RandRange(1, ValidMaterialCount) == 1)
				{
					ChosenIndex = Index;
				}
			}
		}
	}

	if (ChosenIndex == INDEX_NONE)
	{
		UE_LOG(LogLoop9, Warning, TEXT("MaterialSwapAnomaly on '%s': no valid anomaly materials"),
			GetOwner() ? *GetOwner()->GetActorNameOrLabel() : TEXT("None"));
		return false;
	}

	UMaterialInterface* Chosen = AnomalyMaterials[ChosenIndex];
	TargetMesh->SetMaterial(MaterialSlot, Chosen);

	UE_LOG(LogLoop9, Verbose, TEXT("MaterialSwapAnomaly on '%s': slot %d -> %s (idx %d)"),
		GetOwner() ? *GetOwner()->GetActorNameOrLabel() : TEXT("None"),
		MaterialSlot,
		*Chosen->GetName(),
		ChosenIndex);

	return true;
}

void UMaterialSwapAnomalyComponent::RestoreNormalState()
{
	if (IsValid(TargetMesh) && bNormalMaterialCaptured)
	{
		TargetMesh->SetMaterial(MaterialSlot, NormalMaterial);
	}
}
