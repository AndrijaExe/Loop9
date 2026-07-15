#include "Interaction/InspectableComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/InspectionStageActor.h"
#include "Loop9.h"

UInspectableComponent::UInspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UStaticMesh* UInspectableComponent::ResolveMesh(TArray<UMaterialInterface*>& OutMaterialOverrides) const
{
	OutMaterialOverrides.Reset();

	if (MeshOverride)
	{
		return MeshOverride;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (const UStaticMeshComponent* MeshComp = Owner->FindComponentByClass<UStaticMeshComponent>())
	{
		if (UStaticMesh* Mesh = MeshComp->GetStaticMesh())
		{
			// Keep per-instance material overrides so the copy looks identical.
			OutMaterialOverrides = MeshComp->GetMaterials();
			return Mesh;
		}
	}

	return nullptr;
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
	return true;
}

void UInspectableComponent::NotifyInspectionEnded()
{
	UE_LOG(LogLoop9, Log, TEXT("Inspection ended for '%s'"), *GetNameSafe(GetOwner()));
	OnInspectionEnded.Broadcast();
}
