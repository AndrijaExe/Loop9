#include "Anomaly/MaterialSwapAnomalyComponent.h"

#include "Loop9.h"
#include "Algo/Count.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

UMaterialSwapAnomalyComponent::UMaterialSwapAnomalyComponent()
{
	// A repainted surface is the anomaly this game is actually about: it rewards
	// looking rather than reacting, and it survives being checked twice. It
	// shares the Text pool with the spawned-note anomaly, so the weight is what
	// makes the swap the usual winner of that draw.
	AnomalyProbability = 0.7f;
	SelectionWeight = 2.0f;
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
		// Only variants that differ from the baseline are eligible. Refusing
		// here is deliberate: the manager then draws another placement, so a
		// half-authored swap costs the floor nothing instead of turning it into
		// an anomaly the player cannot possibly see.
		const TArray<int32> VisibleIndices = CollectVisibleVariantIndices();
		if (VisibleIndices.Num() > 0)
		{
			ChosenIndex = VisibleIndices[FMath::RandRange(0, VisibleIndices.Num() - 1)];
		}
	}

	if (ChosenIndex == INDEX_NONE)
	{
		UE_LOG(LogLoop9, Warning,
			TEXT("MaterialSwapAnomaly on '%s': no variant differs from the normal material, skipping (variants=%d). Run AnomalyAuditMaterials."),
			GetOwner() ? *GetOwner()->GetActorNameOrLabel() : TEXT("None"),
			AnomalyMaterials.Num());
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

UMaterialInterface* UMaterialSwapAnomalyComponent::ResolveBaselineMaterial() const
{
	// While the anomaly is applied the slot holds a variant, so the captured
	// material is the only honest baseline.
	if (bNormalMaterialCaptured)
	{
		return NormalMaterial;
	}

	return IsValid(TargetMesh) && TargetMesh->GetNumMaterials() > MaterialSlot && MaterialSlot >= 0
		? TargetMesh->GetMaterial(MaterialSlot)
		: nullptr;
}

TArray<int32> UMaterialSwapAnomalyComponent::CollectVisibleVariantIndices() const
{
	const UMaterialInterface* Baseline = ResolveBaselineMaterial();

	TArray<int32> Indices;
	for (int32 Index = 0; Index < AnomalyMaterials.Num(); ++Index)
	{
		UMaterialInterface* Variant = AnomalyMaterials[Index];
		if (IsValid(Variant) && Variant != Baseline)
		{
			Indices.Add(Index);
		}
	}

	return Indices;
}

FString UMaterialSwapAnomalyComponent::DescribeConfigurationProblem() const
{
	if (!IsValid(TargetMesh))
	{
		return TargetComponentName.IsNone()
			? TEXT("owner has no mesh component")
			: FString::Printf(TEXT("no mesh component named '%s' on the owner"), *TargetComponentName.ToString());
	}

	if (MaterialSlot < 0 || MaterialSlot >= TargetMesh->GetNumMaterials())
	{
		return FString::Printf(TEXT("material slot %d is out of range (mesh has %d)"),
			MaterialSlot, TargetMesh->GetNumMaterials());
	}

	if (AnomalyMaterials.IsEmpty())
	{
		return TEXT("no anomaly materials assigned");
	}

	const int32 NullCount = Algo::CountIf(AnomalyMaterials,
		[](const TObjectPtr<UMaterialInterface>& Material) { return !IsValid(Material); });

	if (CollectVisibleVariantIndices().IsEmpty())
	{
		return NullCount == AnomalyMaterials.Num()
			? TEXT("every anomaly material entry is empty")
			: TEXT("every anomaly material equals the normal material, so the swap would be invisible");
	}

	if (NullCount > 0)
	{
		return FString::Printf(TEXT("%d of %d anomaly material entries are empty"),
			NullCount, AnomalyMaterials.Num());
	}

	return FString();
}
