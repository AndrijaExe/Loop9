#include "Interaction/InspectionStageActor.h"

#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Scene.h"
#include "Engine/StaticMesh.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/InspectableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Loop9.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Only one inspection can run at a time (single-player game).
	TWeakObjectPtr<AInspectionStageActor> GActiveInspection;

	// Ignore exit keys right after opening so the interact press that started
	// the inspection doesn't close it in the same or next frame.
	constexpr float ExitInputGraceSeconds = 0.25f;

	// After closing with E, Enhanced Input still fires Interact on the same press —
	// block re-open briefly so the item isn't inspected again immediately.
	constexpr float ReinspectCooldownSeconds = 0.45f;
	double GReinspectBlockedUntilRealtime = 0.0;

	// Right-stick rotation speed, degrees per second at full deflection.
	constexpr float GamepadRotateDegPerSec = 160.0f;
	constexpr float DefaultRotationSpeed = 0.6f;

	// The black room floats high above the map: height fog thins out with
	// altitude (it gets denser downwards), so up there nothing tints the walls.
	constexpr float StageAltitudeOffset = 30000.0f;

	// How far the stage camera sits from the item pivot.
	constexpr float CameraDistanceCm = 45.0f;
	constexpr float CameraRadiusSafetyMultiplier = 1.5f;
}

AInspectionStageActor::AInspectionStageActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// The whole point: keep ticking (input + rotation) while the game is paused.
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ItemPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ItemPivot"));
	ItemPivot->SetupAttachment(Root);

	DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
	DisplayMesh->SetupAttachment(ItemPivot);
	DisplayMesh->SetCollisionProfileName(TEXT("NoCollision"));
	DisplayMesh->SetGenerateOverlapEvents(false);
	DisplayMesh->CastShadow = false;
	// Lighting channel 1 only: the level's sun/skylight (channel 0) cannot
	// touch the item — it is lit exclusively by the stage lights below.
	DisplayMesh->SetLightingChannels(false, true, false);

	// Black room: six inward-facing planes forming a 40 m box. Plain positive
	// scale — a negative-scale (inverted) cube confuses Lumen's surface cache
	// and produced magenta blocks on screen.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Wall planes overlap beyond the room corners so the 90-degree camera
	// frustum cannot expose seams (the camera sits behind the room center).
	constexpr float RoomHalfSizeCm = 3000.0f;
	constexpr float WallOverlapCm = 500.0f;
	constexpr float PlaneBaseSizeCm = 100.0f;
	const float WallScale =
		(2.0f * (RoomHalfSizeCm + WallOverlapCm)) / PlaneBaseSizeCm;

	struct FWallDef
	{
		const TCHAR* Name;
		FVector Location;
		FRotator Rotation; // Engine plane faces +Z; rotate so it faces the room center.
	};
	const FWallDef Walls[6] = {
		{ TEXT("WallFloor"),   FVector(0, 0, -RoomHalfSizeCm), FRotator(0, 0, 0) },
		{ TEXT("WallCeiling"), FVector(0, 0, RoomHalfSizeCm),  FRotator(180, 0, 0) },
		{ TEXT("WallPosX"),    FVector(RoomHalfSizeCm, 0, 0),  FRotator(90, 0, 0) },
		{ TEXT("WallNegX"),    FVector(-RoomHalfSizeCm, 0, 0), FRotator(-90, 0, 0) },
		{ TEXT("WallPosY"),    FVector(0, RoomHalfSizeCm, 0),  FRotator(0, 0, -90) },
		{ TEXT("WallNegY"),    FVector(0, -RoomHalfSizeCm, 0), FRotator(0, 0, 90) },
	};

	for (const FWallDef& Wall : Walls)
	{
		UStaticMeshComponent* WallComp = CreateDefaultSubobject<UStaticMeshComponent>(Wall.Name);
		WallComp->SetupAttachment(Root);
		WallComp->SetCollisionProfileName(TEXT("NoCollision"));
		WallComp->SetGenerateOverlapEvents(false);
		WallComp->CastShadow = false;
		// No lighting channels at all: no light in the level (sun, skylight,
		// stage lights) can put even a specular sheen on the walls.
		WallComp->SetLightingChannels(false, false, false);
		WallComp->SetRelativeLocation(Wall.Location);
		WallComp->SetRelativeRotation(Wall.Rotation);
		WallComp->SetRelativeScale3D(FVector(WallScale, WallScale, 1.0f));
		if (PlaneMesh.Succeeded())
		{
			WallComp->SetStaticMesh(PlaneMesh.Object);
		}
		if (ShapeMaterial.Succeeded())
		{
			WallComp->SetMaterial(0, ShapeMaterial.Object);
		}
		BackdropWalls.Add(WallComp);
	}

	// Camera looks down +X at the item sitting on the actor origin.
	StageCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("StageCamera"));
	StageCamera->SetupAttachment(Root);
	StageCamera->SetRelativeLocation(FVector(-CameraDistanceCm, 0.0f, 0.0f));
	StageCamera->PostProcessBlendWeight = 1.0f;

	// Manual exposure: with a mostly-black screen, auto exposure would crank
	// itself up until the item blows out white. Manual is also independent of
	// the project's "extended luminance range" setting, unlike min/max brightness.
	FPostProcessSettings& PP = StageCamera->PostProcessSettings;
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = AEM_Manual;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = ExposureBias;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.6f;

	// The black room only needs direct lighting. Lumen GI/reflections have
	// nothing useful to compute here and their surface-cache misses were the
	// source of magenta blocks while the item rotated — turn them off.
	PP.bOverride_DynamicGlobalIlluminationMethod = true;
	PP.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
	PP.bOverride_ReflectionMethod = true;
	PP.ReflectionMethod = EReflectionMethod::None;

	// A rotating item + motion blur = permanently soft edges. Kill the blur
	// and add a touch of tonemapper sharpening to counter TAA/TSR softness.
	PP.bOverride_MotionBlurAmount = true;
	PP.MotionBlurAmount = 0.0f;
	PP.bOverride_DepthOfFieldScale = true;
	PP.DepthOfFieldScale = 0.0f;
	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = 0.0f;
	PP.bOverride_FilmGrainIntensity = true;
	PP.FilmGrainIntensity = 0.0f;
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.0f;
	PP.bOverride_Sharpen = true;
	PP.Sharpen = 0.4f;

	// Museum-style lighting: key from upper right near the camera, dim fill
	// from the lower left. Short attenuation so the box walls stay pitch black.
	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(Root);
	KeyLight->SetMobility(EComponentMobility::Movable);
	KeyLight->SetRelativeLocation(FVector(-35.0f, 25.0f, 30.0f));
	KeyLight->SetIntensityUnits(ELightUnits::Candelas);
	KeyLight->SetAttenuationRadius(600.0f);
	KeyLight->SetCastShadows(false);
	KeyLight->SetLightingChannels(false, true, false);

	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(Root);
	FillLight->SetMobility(EComponentMobility::Movable);
	FillLight->SetRelativeLocation(FVector(-35.0f, -30.0f, -15.0f));
	FillLight->SetIntensityUnits(ELightUnits::Candelas);
	FillLight->SetAttenuationRadius(600.0f);
	FillLight->SetCastShadows(false);
	FillLight->SetLightingChannels(false, true, false);
}

bool AInspectionStageActor::IsInspectionActive()
{
	return GActiveInspection.IsValid();
}

bool AInspectionStageActor::TryEndActiveInspection()
{
	if (AInspectionStageActor* Active = GActiveInspection.Get())
	{
		Active->EndInspection();
		return true;
	}

	// Consume the same key press briefly after closing. Without this, the raw
	// paused-tick handler can close inspection before Enhanced Input dispatches
	// OnPause/OnInteract, causing that same press to open the pause menu or
	// immediately start another inspection.
	return FPlatformTime::Seconds() < GReinspectBlockedUntilRealtime;
}

bool AInspectionStageActor::BeginInspection(UInspectableComponent* SourceComponent, APlayerController* InController)
{
	if (!SourceComponent || !InController || GActiveInspection.IsValid())
	{
		return false;
	}

	if (FPlatformTime::Seconds() < GReinspectBlockedUntilRealtime)
	{
		return false;
	}

	// Don't fight the pause menu (or another system) over the pause state.
	if (UGameplayStatics::IsGamePaused(GetWorld()))
	{
		return false;
	}

	TArray<UMaterialInterface*> MaterialOverrides;
	UStaticMesh* Mesh = SourceComponent->ResolveMesh(MaterialOverrides);
	if (!Mesh)
	{
		UE_LOG(LogLoop9, Warning, TEXT("InspectionStage: '%s' has no static mesh to inspect."),
			*GetNameSafe(SourceComponent->GetOwner()));
		return false;
	}

	Source = SourceComponent;
	Controller = InController;
	RotationSpeed = SourceComponent->RotationSpeed;

	// Apply the runtime-tunable light/exposure config (values may come from
	// DefaultGame.ini overrides, so set them here rather than in the ctor).
	KeyLight->SetIntensity(KeyLightIntensityCandela);
	FillLight->SetIntensity(FillLightIntensityCandela);
	StageCamera->PostProcessSettings.AutoExposureBias = ExposureBias;

	// BasicShapeMaterial is a hard constructor reference, so it is cooked in
	// Shipping builds. Avoid EngineDebugMaterials: runtime LoadObject paths to
	// debug assets are not guaranteed to be included by the cooker.
	UMaterialInterface* WallMaterial = nullptr;
	if (BackdropWalls.Num() > 0 && BackdropWalls[0]->GetMaterial(0))
	{
		UMaterialInstanceDynamic* BackdropMID =
			UMaterialInstanceDynamic::Create(BackdropWalls[0]->GetMaterial(0), this);
		BackdropMID->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black);
		WallMaterial = BackdropMID;
	}
	if (WallMaterial)
	{
		for (UStaticMeshComponent* Wall : BackdropWalls)
		{
			Wall->SetMaterial(0, WallMaterial);
		}
	}

	// --- Display mesh setup ---
	DisplayMesh->SetStaticMesh(Mesh);
	for (int32 i = 0; i < MaterialOverrides.Num(); ++i)
	{
		if (MaterialOverrides[i])
		{
			DisplayMesh->SetMaterial(i, MaterialOverrides[i]);
			const EBlendMode BlendMode = MaterialOverrides[i]->GetBlendMode();
			if (BlendMode != BLEND_Opaque && BlendMode != BLEND_Masked)
			{
				UE_LOG(LogLoop9, Warning,
					TEXT("InspectionStage: material slot %d on '%s' is translucent. "
						 "Use InspectionMaterialOverrides for a fully opaque inspection view."),
					i, *GetNameSafe(SourceComponent->GetOwner()));
			}
		}
	}

	// Normalize the size: whatever the source item is, show it at a
	// comfortable, consistent size in front of the camera.
	const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
	const float BoundsRadius = FMath::Max(MeshBounds.SphereRadius, 1.0f);
	const float Scale = SourceComponent->TargetRadiusCm / BoundsRadius;
	DisplayMesh->SetRelativeScale3D(FVector(Scale));

	// Keep the full normalized bounding sphere in front of the near plane.
	// DistanceCm remains a preferred framing distance, not an unsafe promise.
	const float SafeCameraDistance = FMath::Max(
		FMath::Max(SourceComponent->DistanceCm, 20.0f),
		SourceComponent->TargetRadiusCm * CameraRadiusSafetyMultiplier);
	StageCamera->SetRelativeLocation(FVector(-SafeCameraDistance, 0.0f, 0.0f));

	// Offset the mesh so its bounds center sits on the pivot — rotating the
	// pivot then spins the item around its visual center, regardless of
	// where the mesh pivot is.
	DisplayMesh->SetRelativeLocation(-MeshBounds.Origin * Scale);

	ItemPivot->SetRelativeRotation(SourceComponent->InitialRotationOffset);

	// --- Placement: park the whole room high above the player, in the void ---
	const FVector PlayerLocation = InController->GetPawn()
		? InController->GetPawn()->GetActorLocation()
		: FVector::ZeroVector;
	SetActorLocation(PlayerLocation + FVector(0.0f, 0.0f, StageAltitudeOffset));
	SetActorRotation(FRotator::ZeroRotator);

	// --- Switch the view to the stage camera ---
	PreviousViewTarget = InController->GetViewTarget();
	InController->SetViewTargetWithBlend(this, 0.0f);
	if (APlayerCameraManager* CameraManager = InController->PlayerCameraManager)
	{
		// The view teleports 300 m. Mark a camera cut so TSR/TAA does not
		// reproject stale gameplay history into purple blocks around the item.
		CameraManager->SetGameCameraCutThisFrame();
	}

	// --- Pause ---
	// The camera manager normally skips its update while the game is paused,
	// which would leave the old view on screen — full tick keeps it running.
	bPreviousFullTickWhenPaused = InController->bShouldPerformFullTickWhenPaused;
	InController->bShouldPerformFullTickWhenPaused = true;

	bDidPause = UGameplayStatics::SetGamePaused(GetWorld(), true);

	// Belt and braces: full tick above also processes look input while
	// paused, so silence it; RestoreState pairs the decrement exactly once.
	InController->SetIgnoreLookInput(true);
	InController->SetIgnoreMoveInput(true);
	bDidIgnoreInput = true;

	GActiveInspection = this;
	bActive = true;
	TimeActive = 0.0f;
	// Treat opening keys as already held so the interact press that opened
	// the view cannot immediately close it on the rising-edge check.
	bExitKeyWasDown = IsExitKeyDown();
	SetActorTickEnabled(true);

	return true;
}

void AInspectionStageActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bActive)
	{
		return;
	}

	APlayerController* PC = Controller.Get();
	if (!PC)
	{
		EndInspection();
		return;
	}

	TimeActive += DeltaSeconds;

	const bool bExitDown = IsExitKeyDown();
	if (TimeActive > ExitInputGraceSeconds && bExitDown && !bExitKeyWasDown)
	{
		EndInspection();
		return;
	}
	bExitKeyWasDown = bExitDown;

	// --- Rotation input: mouse always rotates; right stick on gamepad ---
	float DeltaX = 0.0f;
	float DeltaY = 0.0f;
	PC->GetInputMouseDelta(DeltaX, DeltaY);

	// Raw analog values need a deadzone, or a worn stick slowly spins the item.
	auto WithDeadzone = [](float Value)
	{
		return FMath::Abs(Value) > 0.15f ? Value : 0.0f;
	};
	// Normalize against the default so RotationSpeed scales mouse and gamepad
	// consistently while preserving 160 deg/s at the default 0.6 value.
	DeltaX += WithDeadzone(PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX))
		* GamepadRotateDegPerSec * DeltaSeconds / DefaultRotationSpeed;
	DeltaY += WithDeadzone(PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY))
		* GamepadRotateDegPerSec * DeltaSeconds / DefaultRotationSpeed;

	if (FMath::IsNearlyZero(DeltaX) && FMath::IsNearlyZero(DeltaY))
	{
		return;
	}

	// "Grab" feel: dragging right pulls the front face to the right,
	// dragging up tips the top away from the player. The stage camera looks
	// down +X with no roll, so camera up is world Z and camera right is +Y.
	const FQuat YawDelta(FVector::UpVector, FMath::DegreesToRadians(-DeltaX * RotationSpeed));
	const FQuat PitchDelta(FVector::RightVector, FMath::DegreesToRadians(-DeltaY * RotationSpeed));
	ItemPivot->SetWorldRotation((YawDelta * PitchDelta * ItemPivot->GetComponentQuat()).Rotator());
}

bool AInspectionStageActor::IsExitKeyDown() const
{
	const APlayerController* PC = Controller.Get();
	if (!PC)
	{
		return true;
	}

	return PC->IsInputKeyDown(EKeys::Escape)
		|| PC->IsInputKeyDown(EKeys::E)
		|| PC->IsInputKeyDown(EKeys::RightMouseButton)
		|| PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Right)
		|| PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Left);
}

void AInspectionStageActor::EndInspection()
{
	RestoreState();
	Destroy();
}

void AInspectionStageActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Safety net: level teardown or external destroy while inspecting.
	RestoreState();
	Super::EndPlay(EndPlayReason);
}

void AInspectionStageActor::RestoreState()
{
	if (bRestored)
	{
		return;
	}
	const bool bWasActive = bActive;
	bRestored = true;
	bActive = false;
	SetActorTickEnabled(false);

	APlayerController* PC = Controller.Get();

	// Put the view back where it was (while the controller still fully ticks,
	// so the camera manager applies the switch even though the game is paused).
	if (PC)
	{
		AActor* ViewTarget = PreviousViewTarget.Get();
		if (!ViewTarget)
		{
			ViewTarget = PC->GetPawn();
		}
		if (!ViewTarget)
		{
			// Avoid leaving the camera manager targeting this stage while it is
			// being destroyed during pawn death or level teardown.
			ViewTarget = PC;
		}
		if (ViewTarget)
		{
			PC->SetViewTargetWithBlend(ViewTarget, 0.0f);
			if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
			{
				CameraManager->SetGameCameraCutThisFrame();
			}
		}
	}

	if (bDidPause)
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
	}

	if (PC)
	{
		PC->bShouldPerformFullTickWhenPaused = bPreviousFullTickWhenPaused;
	}

	if (bDidIgnoreInput)
	{
		if (PC)
		{
			PC->SetIgnoreLookInput(false);
			PC->SetIgnoreMoveInput(false);
		}
		bDidIgnoreInput = false;
	}

	// Failed BeginInspection attempts also destroy the stage actor and reach
	// EndPlay. Only a session that actually opened may consume input/cool down.
	if (bWasActive)
	{
		GReinspectBlockedUntilRealtime = FPlatformTime::Seconds() + ReinspectCooldownSeconds;
	}

	if (GActiveInspection.Get() == this)
	{
		GActiveInspection.Reset();
	}

	if (bWasActive)
	{
		if (UInspectableComponent* SourceComponent = Source.Get())
		{
			SourceComponent->NotifyInspectionEnded();
		}
	}
}
