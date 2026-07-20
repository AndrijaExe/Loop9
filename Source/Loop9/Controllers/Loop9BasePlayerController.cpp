#include "Loop9BasePlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Loop9Character.h"
#include "Interaction/Loop9Interactable.h"
#include "Interaction/InspectableComponent.h"
#include "Interaction/InteractionPromptProvider.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ALoop9BasePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
	}
}

void ALoop9BasePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoopManagerSubsystem* LoopManager = GameInstance->GetSubsystem<ULoopManagerSubsystem>();
			LoopManager
			&& (LoopManager->IsElevatorTransitionPending() || LoopManager->bGameFinished))
		{
			InteractionPromptTraceAccumulator = 0.0f;
			PushPromptToUI(FText::GetEmpty(), false);
			return;
		}
	}

	// Always clear immediately while paused; throttle expensive complex traces otherwise.
	if (UGameplayStatics::IsGamePaused(GetWorld()))
	{
		InteractionPromptTraceAccumulator = 0.0f;
		PushPromptToUI(FText::GetEmpty(), false);
		return;
	}

	const float TraceInterval = 1.0f / FMath::Max(1.0f, InteractionPromptTraceHz);
	InteractionPromptTraceAccumulator += DeltaSeconds;
	if (InteractionPromptTraceAccumulator < TraceInterval)
	{
		return;
	}
	InteractionPromptTraceAccumulator = 0.0f;

	UpdateInteractionPrompt();
}

void ALoop9BasePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}

		if (!SVirtualJoystick::ShouldDisplayTouchInterface())
		{
			for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}

void ALoop9BasePlayerController::ClearInteractionPrompt()
{
	PushPromptToUI(FText::GetEmpty(), false);
}

void ALoop9BasePlayerController::UpdateInteractionPrompt()
{
	ALoop9Character* LoopCharacter = Cast<ALoop9Character>(GetPawn());
	if (!LoopCharacter || !GetInteractionPromptWidget())
	{
		PushPromptToUI(FText::GetEmpty(), false);
		return;
	}

	UCameraComponent* Camera = LoopCharacter->GetFirstPersonCameraComponent();
	if (!Camera)
	{
		PushPromptToUI(FText::GetEmpty(), false);
		return;
	}

	const FVector TraceStart = Camera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * InteractionPromptDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(Loop9InteractionPromptTrace), true, LoopCharacter);
	QueryParams.bTraceComplex = true;

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	if (!bHit || !HitResult.GetActor())
	{
		PushPromptToUI(FText::GetEmpty(), false);
		return;
	}

	const FText Prompt = ResolvePromptForActor(HitResult.GetActor());
	PushPromptToUI(Prompt, !Prompt.IsEmpty());
}

FText ALoop9BasePlayerController::ResolvePromptForActor(AActor* HitActor) const
{
	if (!HitActor)
	{
		return FText::GetEmpty();
	}

	if (HitActor->GetClass()->ImplementsInterface(ULoop9Interactable::StaticClass()))
	{
		const FText InterfacePrompt =
			ILoop9Interactable::Execute_GetInteractionPromptText(HitActor);
		if (!InterfacePrompt.IsEmpty())
		{
			return InterfacePrompt;
		}
	}

	// Actors with an InspectableComponent are examinable without the interface.
	if (const UInspectableComponent* Inspectable = HitActor->FindComponentByClass<UInspectableComponent>())
	{
		return Inspectable->PromptText;
	}

	return FText::GetEmpty();
}

void ALoop9BasePlayerController::PushPromptToUI(const FText& PromptText, bool bVisible)
{
	UUserWidget* PromptWidget = GetInteractionPromptWidget();
	if (!PromptWidget)
	{
		return;
	}

	if (bVisible == bLastPromptVisible && PromptText.EqualTo(LastPromptText))
	{
		return;
	}

	bLastPromptVisible = bVisible;
	LastPromptText = PromptText;

	if (PromptWidget->GetClass()->ImplementsInterface(UInteractionPromptProvider::StaticClass()))
	{
		IInteractionPromptProvider::Execute_SetInteractionPrompt(PromptWidget, PromptText, bVisible);
	}
}
