#include "Interaction/LiftButton.h"

#include "Interaction/LoopElevatorTransitionDirector.h"
#include "LiftDoorWing.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"

ALiftButton::ALiftButton()
{
	PrimaryActorTick.bCanEverTick = false;

	ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
	SetRootComponent(ButtonMesh);

	IndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("IndicatorLight"));
	IndicatorLight->SetupAttachment(ButtonMesh);
	IndicatorLight->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f));
	IndicatorLight->SetIntensityUnits(ELightUnits::Lumens);
	IndicatorLight->SetAttenuationRadius(350.0f);
	IndicatorLight->SetCastShadows(false);

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(ButtonMesh);
	LabelText->SetRelativeLocation(LabelRelativeOffset);
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextCenter);
	LabelText->SetWorldSize(LabelWorldSize);
	LabelText->SetTextRenderColor(FColor::White);
	LabelText->SetVisibility(false);
	LabelText->SetHiddenInGame(true);

	CabinVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CabinVolume"));
	CabinVolume->SetupAttachment(ButtonMesh);
	// Disabled by default — door-plane gate covers maps without authored volumes.
	CabinVolume->SetBoxExtent(FVector(1.0f, 1.0f, 1.0f));
	CabinVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CabinVolume->SetGenerateOverlapEvents(false);
	CabinVolume->SetHiddenInGame(true);
}

void ALiftButton::BeginPlay()
{
	Super::BeginPlay();
	ApplyIndicatorVisuals();
}

void ALiftButton::ApplyIndicatorVisuals()
{
	const bool bIsResetButton = ButtonType == ELiftButtonType::Reset;
	const float TargetIntensity = bIsResetButton ? ActiveLightIntensity : InactiveLightIntensity;
	const FLinearColor TargetColor = bIsResetButton ? ActiveLightColor : InactiveLightColor;
	const float TargetEmissive = bIsResetButton ? EmissiveStrengthWhenActive : EmissiveStrengthWhenInactive;

	if (IndicatorLight)
	{
		IndicatorLight->SetVisibility(bEnableIndicatorLight);
		IndicatorLight->SetIntensity(bEnableIndicatorLight ? TargetIntensity : 0.0f);
		IndicatorLight->SetLightColor(TargetColor);
	}

	UpdateMeshEmissive(TargetEmissive, TargetColor);

	if (LabelText)
	{
		const bool bHasLabel = !WorldLabelText.IsEmpty();
		LabelText->SetVisibility(bHasLabel);
		LabelText->SetHiddenInGame(!bHasLabel);
		LabelText->SetWorldSize(LabelWorldSize);
		LabelText->SetRelativeLocation(LabelRelativeOffset);

		if (bHasLabel)
		{
			LabelText->SetText(WorldLabelText);
			LabelText->SetTextRenderColor(bIsResetButton ? FColor(255, 220, 140) : FColor(170, 190, 255));
		}
	}
}

void ALiftButton::UpdateMeshEmissive(float Strength, const FLinearColor& Tint) const
{
	if (!ButtonMesh)
	{
		return;
	}

	const int32 MaterialCount = ButtonMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInstanceDynamic* DynamicMaterial = ButtonMesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
		if (!DynamicMaterial)
		{
			continue;
		}

		DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), Strength);
		DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), Tint);
		DynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), Tint * Strength);
	}
}

void ALiftButton::Interact()
{
	HandleInteraction(UGameplayStatics::GetPlayerController(GetWorld(), 0));
}

bool ALiftButton::IsPlayerInsideCabin(APlayerController* InteractingController) const
{
	if (!bRequirePlayerInsideCabin)
	{
		return true;
	}

	const APawn* Pawn = InteractingController ? InteractingController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return false;
	}

	const FVector PlayerLocation = Pawn->GetActorLocation();
	const float MaxButtonDistance = FMath::Max(50.0f, CabinMaxDistanceFromButtonCm);
	if (FVector::Dist(PlayerLocation, GetActorLocation()) > MaxButtonDistance)
	{
		return false;
	}

	if (bUseCabinVolumeGate && CabinVolume)
	{
		const FVector Extent = CabinVolume->GetScaledBoxExtent();
		if (Extent.GetMax() > 1.5f)
		{
			const FTransform VolumeTransform = CabinVolume->GetComponentTransform();
			const FVector Local = VolumeTransform.InverseTransformPosition(PlayerLocation);
			return FMath::Abs(Local.X) <= Extent.X
				&& FMath::Abs(Local.Y) <= Extent.Y
				&& FMath::Abs(Local.Z) <= Extent.Z;
		}
	}

	FVector DoorCenter = FVector::ZeroVector;
	int32 DoorCount = 0;
	for (const TObjectPtr<ALiftDoorWing>& DoorWing : TransitionDoorWings)
	{
		if (IsValid(DoorWing))
		{
			DoorCenter += DoorWing->GetActorLocation();
			++DoorCount;
		}
	}

	if (DoorCount <= 0)
	{
		// No door refs: proximity to the button is the only available gate.
		return true;
	}

	DoorCenter /= static_cast<float>(DoorCount);
	FVector Inward = (GetActorLocation() - DoorCenter).GetSafeNormal2D();
	if (Inward.IsNearlyZero())
	{
		return true;
	}

	const float DepthPastDoors = FVector::DotProduct(PlayerLocation - DoorCenter, Inward);
	return DepthPastDoors >= CabinMinDepthPastDoorsCm;
}

bool ALiftButton::HandleInteraction(APlayerController* InteractingController)
{
	if (!IsPlayerInsideCabin(InteractingController))
	{
		UE_LOG(LogTemp, Log, TEXT("LiftButton: ignored press — player is outside the cabin."));
		return false;
	}

	if (IsValid(TransitionDirector))
	{
		if (TransitionDirector->IsTransitionInProgress())
		{
			return true;
		}

		if (TransitionDirector->BeginTransition(this, InteractingController))
		{
			OnInteracted();
			return true;
		}
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (!GameInstance)
	{
		return false;
	}

	ULoopManagerSubsystem* LoopManager = GameInstance->GetSubsystem<ULoopManagerSubsystem>();
	if (!LoopManager)
	{
		return false;
	}

	const EButtonType PressedType = ButtonType == ELiftButtonType::Increment
		? EButtonType::Increment
		: EButtonType::Reset;

	LoopManager->OnElevatorButtonPressed(PressedType);
	OnInteracted();
	return true;
}

bool ALiftButton::TryInteract_Implementation(APlayerController* InteractingController)
{
	return HandleInteraction(InteractingController);
}

FText ALiftButton::GetInteractionPromptText_Implementation() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (bRequirePlayerInsideCabin && PC && !IsPlayerInsideCabin(PC))
	{
		return NSLOCTEXT("Loop9Interaction", "LiftEnterCabinFirst", "Enter the elevator first");
	}

	if (!InteractionPromptText.IsEmpty())
	{
		return InteractionPromptText;
	}

	return ButtonType == ELiftButtonType::Reset
		? NSLOCTEXT("Loop9Interaction", "LiftRestart", "Restart loop")
		: NSLOCTEXT("Loop9Interaction", "LiftNext", "Next floor");
}
