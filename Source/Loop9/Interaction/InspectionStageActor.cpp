#include "Interaction/InspectionStageActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/InspectableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Loop9.h"
#include "Loop9Character.h"
#include "Materials/MaterialInterface.h"

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
}

AInspectionStageActor::AInspectionStageActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// The whole point: keep ticking (input + rotation) while the game is paused.
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
	DisplayMesh->SetupAttachment(Root);
	DisplayMesh->SetCollisionProfileName(TEXT("NoCollision"));
	DisplayMesh->SetGenerateOverlapEvents(false);
	DisplayMesh->CastShadow = false;
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

	ALoop9Character* Character = Cast<ALoop9Character>(InController->GetPawn());
	if (!Character)
	{
		return false;
	}

	UCameraComponent* Cam = Character->GetFirstPersonCameraComponent();
	if (!Cam)
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
	Camera = Cam;
	RotationSpeed = SourceComponent->RotationSpeed;

	// --- Display mesh setup ---
	DisplayMesh->SetStaticMesh(Mesh);
	for (int32 i = 0; i < MaterialOverrides.Num(); ++i)
	{
		if (MaterialOverrides[i])
		{
			DisplayMesh->SetMaterial(i, MaterialOverrides[i]);
		}
	}

	// Preferred path: a post-process material (configured via
	// BackgroundBlurMaterialPath in DefaultGame.ini) blurs everything except
	// pixels the item marks through the custom depth stencil. The item stays
	// perfectly sharp — real depth of field can never do that for an object
	// this close to the camera: its edges always catch part of the blur no
	// matter the aperture. Recipe for the material: EDITOR_TODO.md §4b.
	UMaterialInterface* BlurMaterial = Cast<UMaterialInterface>(BackgroundBlurMaterialPath.TryLoad());

	if (BlurMaterial)
	{
		// Render as a first-person primitive (same trick as the player arms):
		// drawn with the camera's first-person scale, so it does not clip
		// into nearby walls. Safe here because no depth-based effect is used.
		DisplayMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
		DisplayMesh->SetRenderCustomDepth(true);
		DisplayMesh->SetCustomDepthStencilValue(1);
	}
	else
	{
		// DOF fallback needs the item's rendered depth to be exactly
		// DistanceCm so the focal plane is guaranteed to match — no
		// first-person scale trick here.
		DisplayMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::None;
	}

	// Normalize the size: whatever the source item is, show it at a
	// comfortable, consistent size in front of the camera.
	const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
	const float BoundsRadius = FMath::Max(MeshBounds.SphereRadius, 1.0f);
	const float Scale = SourceComponent->TargetRadiusCm / BoundsRadius;
	DisplayMesh->SetWorldScale3D(FVector(Scale));

	// Offset the mesh so its bounds center sits at the actor origin —
	// rotating the actor then spins the item around its visual center,
	// regardless of where the mesh pivot is.
	DisplayMesh->SetRelativeLocation(-MeshBounds.Origin * Scale);

	// --- Placement ---
	const FVector CamLocation = Cam->GetComponentLocation();
	const FVector CamForward = Cam->GetForwardVector();
	SetActorLocation(CamLocation + CamForward * SourceComponent->DistanceCm);

	// Start facing the camera the same way the item faced in the world,
	// then apply the per-item presentation offset.
	FRotator StartRotation = Cam->GetComponentRotation();
	StartRotation.Pitch = 0.0f;
	StartRotation.Roll = 0.0f;
	SetActorRotation(StartRotation);
	AddActorLocalRotation(SourceComponent->InitialRotationOffset);

	// --- Background separation (blur) + darker vignette ---
	SavedPostProcess = Cam->PostProcessSettings;
	FPostProcessSettings& PP = Cam->PostProcessSettings;

	if (BlurMaterial)
	{
		// Stencil-masked screen blur: background fully blurred, item untouched.
		// Restoring SavedPostProcess removes the blendable again.
		PP.AddBlendable(BlurMaterial, 1.0f);
	}
	else
	{
		// Fallback: cinematic depth of field focused exactly at the item's
		// depth. Moderate aperture — strong enough to soften the room, mild
		// enough that the item's own thickness stays acceptable. (FocalRegion
		// and transition-region settings are Gaussian/mobile-only; they do
		// nothing with desktop cinematic DOF, so they are not set here.)
		PP.bOverride_DepthOfFieldFocalDistance = true;
		PP.DepthOfFieldFocalDistance = SourceComponent->DistanceCm;
		PP.bOverride_DepthOfFieldFstop = true;
		PP.DepthOfFieldFstop = 2.0f;
		PP.bOverride_DepthOfFieldMinFstop = true;
		PP.DepthOfFieldMinFstop = 2.0f;
	}

	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.9f;
	// Slightly dimmer than gameplay, but not as crushed as the -2.75 pass.
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = -1.5f;
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(0.85f, 0.85f, 0.85f, 1.0f);
	PP.bOverride_ColorGain = true;
	PP.ColorGain = FVector4(0.82f, 0.82f, 0.84f, 1.0f);

	// --- Hide the first-person arms so they don't overlap the item ---
	if (USkeletalMeshComponent* Arms = Character->GetFirstPersonMesh())
	{
		if (Arms->IsVisible())
		{
			Arms->SetVisibility(false, false);
			HiddenArms = Arms;
		}
	}

	// --- Pause ---
	bDidPause = UGameplayStatics::SetGamePaused(GetWorld(), true);

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

	DeltaX += PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX) * GamepadRotateDegPerSec * DeltaSeconds / FMath::Max(RotationSpeed, 0.05f);
	DeltaY += PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY) * GamepadRotateDegPerSec * DeltaSeconds / FMath::Max(RotationSpeed, 0.05f);

	if (FMath::IsNearlyZero(DeltaX) && FMath::IsNearlyZero(DeltaY))
	{
		return;
	}

	const UCameraComponent* Cam = Camera.Get();
	if (!Cam)
	{
		return;
	}

	// "Grab" feel: dragging right pulls the front face to the right,
	// dragging up tips the top away from the player.
	const FQuat YawDelta(Cam->GetUpVector(), FMath::DegreesToRadians(-DeltaX * RotationSpeed));
	const FQuat PitchDelta(Cam->GetRightVector(), FMath::DegreesToRadians(-DeltaY * RotationSpeed));
	SetActorRotation((YawDelta * PitchDelta * GetActorQuat()).Rotator());
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

	if (UCameraComponent* Cam = Camera.Get())
	{
		Cam->PostProcessSettings = SavedPostProcess;
	}

	if (USkeletalMeshComponent* Arms = HiddenArms.Get())
	{
		Arms->SetVisibility(true, false);
	}

	if (bDidPause)
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
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
