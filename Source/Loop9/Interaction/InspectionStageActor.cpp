#include "Interaction/InspectionStageActor.h"

#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Scene.h"
#include "Engine/StaticMesh.h"
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

	// The black room floats high above the map: height fog thins out with
	// altitude (it gets denser downwards), so up there nothing tints the walls.
	constexpr float StageAltitudeOffset = 30000.0f;

	// How far the stage camera sits from the item pivot.
	constexpr float CameraDistanceCm = 45.0f;
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

	Backdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(Root);
	Backdrop->SetCollisionProfileName(TEXT("NoCollision"));
	Backdrop->SetGenerateOverlapEvents(false);
	Backdrop->CastShadow = false;
	// Negative uniform scale flips the winding, so the cube is visible from
	// the inside — a 40 m box enclosing the whole stage.
	Backdrop->SetRelativeScale3D(FVector(-40.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Backdrop->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMaterial.Succeeded())
	{
		Backdrop->SetMaterial(0, ShapeMaterial.Object);
	}

	// Camera looks down +X at the item sitting on the actor origin.
	StageCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("StageCamera"));
	StageCamera->SetupAttachment(Root);
	StageCamera->SetRelativeLocation(FVector(-CameraDistanceCm, 0.0f, 0.0f));

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
	return false;
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

	// Respect the per-item framing distance.
	StageCamera->SetRelativeLocation(FVector(-FMath::Max(SourceComponent->DistanceCm, 20.0f), 0.0f, 0.0f));

	// Apply the runtime-tunable light/exposure config (values may come from
	// DefaultGame.ini overrides, so set them here rather than in the ctor).
	KeyLight->SetIntensity(KeyLightIntensityCandela);
	FillLight->SetIntensity(FillLightIntensityCandela);
	StageCamera->PostProcessSettings.AutoExposureBias = ExposureBias;

	// Pure black walls. Preferred: the engine's unlit black material — unlit
	// means the sun/skylight physically cannot put any sheen on the walls.
	// Fallback: dynamic instance of the shape material with black albedo.
	if (UMaterialInterface* UnlitBlack = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/EngineDebugMaterials/BlackUnlitMaterial.BlackUnlitMaterial")))
	{
		Backdrop->SetMaterial(0, UnlitBlack);
	}
	else if (UMaterialInterface* BackdropBase = Backdrop->GetMaterial(0))
	{
		UMaterialInstanceDynamic* BackdropMID = UMaterialInstanceDynamic::Create(BackdropBase, this);
		BackdropMID->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black);
		Backdrop->SetMaterial(0, BackdropMID);
	}

	// --- Display mesh setup ---
	DisplayMesh->SetStaticMesh(Mesh);
	for (int32 i = 0; i < MaterialOverrides.Num(); ++i)
	{
		if (MaterialOverrides[i])
		{
			DisplayMesh->SetMaterial(i, MaterialOverrides[i]);
		}
	}

	// Normalize the size: whatever the source item is, show it at a
	// comfortable, consistent size in front of the camera.
	const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
	const float BoundsRadius = FMath::Max(MeshBounds.SphereRadius, 1.0f);
	const float Scale = SourceComponent->TargetRadiusCm / BoundsRadius;
	DisplayMesh->SetRelativeScale3D(FVector(Scale));

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
	DeltaX += WithDeadzone(PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX)) * GamepadRotateDegPerSec * DeltaSeconds / FMath::Max(RotationSpeed, 0.05f);
	DeltaY += WithDeadzone(PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY)) * GamepadRotateDegPerSec * DeltaSeconds / FMath::Max(RotationSpeed, 0.05f);

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
		if (ViewTarget)
		{
			PC->SetViewTargetWithBlend(ViewTarget, 0.0f);
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

	// Block immediate re-inspect from the same E that closed the view.
	GReinspectBlockedUntilRealtime = FPlatformTime::Seconds() + ReinspectCooldownSeconds;

	if (GActiveInspection.Get() == this)
	{
		GActiveInspection.Reset();
	}

	if (UInspectableComponent* SourceComponent = Source.Get())
	{
		SourceComponent->NotifyInspectionEnded();
	}
}
