#include "Interaction/LoopEndingSceneDirector.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LiftDoorWing.h"
#include "Loop9.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr uint8 AudioBitFootstep1 = 1 << 0;
	constexpr uint8 AudioBitFootstep2 = 1 << 1;
	constexpr uint8 AudioBitPhoneRing = 1 << 2;
	constexpr uint8 AudioBitLineCut = 1 << 3;
	constexpr uint8 AudioBitFlicker1 = 1 << 4;
	constexpr uint8 AudioBitFlicker2 = 1 << 5;

	constexpr float ReplacementEarlyFinishSeconds = 1.2f;
	constexpr float ObedientExtraDistanceCm = 50.0f;
	constexpr float ColdDoorOpenTimeoutSeconds = 3.0f;
	constexpr float ColdBlackoutDelayAfterDoorsSeconds = 0.5f;
	/** Eyes appear only after full blackout settles. */
	constexpr float ColdEyesDelayAfterBlackoutSeconds = 1.5f;
	constexpr float ColdEyesHoldSeconds = 3.25f;

	float Smooth01(float T)
	{
		T = FMath::Clamp(T, 0.0f, 1.0f);
		return T * T * (3.0f - 2.0f * T);
	}
}

ALoopEndingSceneDirector::ALoopEndingSceneDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SceneCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SceneCamera"));
	SceneCamera->SetupAttachment(Root);
	SceneCamera->SetFieldOfView(70.0f);
	SceneCamera->bConstrainAspectRatio = false;

	WarmKeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WarmKeyLight"));
	WarmKeyLight->SetupAttachment(Root);
	WarmKeyLight->SetVisibility(false);
	WarmKeyLight->SetIntensity(0.0f);
	WarmKeyLight->SetLightColor(FLinearColor(1.0f, 0.82f, 0.55f));
	WarmKeyLight->SetAttenuationRadius(1200.0f);

	AccentLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccentLight"));
	AccentLight->SetupAttachment(Root);
	AccentLight->SetVisibility(false);
	AccentLight->SetIntensity(0.0f);
	AccentLight->SetLightColor(FLinearColor(0.85f, 0.08f, 0.05f));
	AccentLight->SetAttenuationRadius(900.0f);

	EyeLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyeLight"));
	EyeLight->SetupAttachment(Root);
	EyeLight->SetVisibility(false);
	EyeLight->SetIntensity(0.0f);
	EyeLight->SetLightColor(FLinearColor(0.95f, 0.05f, 0.04f));
	EyeLight->SetAttenuationRadius(55.0f);
	EyeLight->SetSourceRadius(2.0f);

	MonitorText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MonitorText"));
	MonitorText->SetupAttachment(Root);
	MonitorText->SetVisibility(false);
	MonitorText->SetHorizontalAlignment(EHTA_Center);
	MonitorText->SetVerticalAlignment(EVRTA_TextCenter);
	MonitorText->SetWorldSize(9.0f);
	MonitorText->SetTextRenderColor(FColor(180, 255, 180));
}

float ALoopEndingSceneDirector::GetSceneDurationSeconds() const
{
	return GetEffectiveSceneDuration();
}

float ALoopEndingSceneDirector::GetEffectiveSceneDuration() const
{
	if (ActiveEnding == ELoopEndingType::TheReplacement)
	{
		return FMath::Max(SceneDurationSeconds - ReplacementEarlyFinishSeconds, 3.5f);
	}
	if (ActiveEnding == ELoopEndingType::ColdBetrayal && bColdEyesShown && ColdEyesShownAtSeconds >= 0.0f)
	{
		return FMath::Max(SceneDurationSeconds, ColdEyesShownAtSeconds + ColdEyesHoldSeconds);
	}
	if (ActiveEnding == ELoopEndingType::ColdBetrayal)
	{
		// Doors + blackout + eye delay + eye hold — don't cut before the eyes beat.
		return FMath::Max(
			SceneDurationSeconds,
			ColdDoorOpenTimeoutSeconds
				+ ColdBlackoutDelayAfterDoorsSeconds
				+ ColdEyesDelayAfterBlackoutSeconds
				+ ColdEyesHoldSeconds);
	}
	return SceneDurationSeconds;
}

bool ALoopEndingSceneDirector::PlayEnding(ELoopEndingType EndingType, APlayerController* InteractingController)
{
	if (bPlaying || !InteractingController)
	{
		return false;
	}

	// The Exit happens outside the office; it has its own Level Sequence
	// (ALoop9GameMode::EndingSequences) or a plain fade, never this desk scene.
	if (EndingType == ELoopEndingType::TheExit)
	{
		return false;
	}

	Controller = InteractingController;
	PreviousViewTarget = InteractingController->GetViewTarget();
	ActiveEnding = EndingType;
	ElapsedSeconds = 0.0f;
	FiredAudioMask = 0;
	FiredLightPulseMask = 0;
	bMonitorTextShown = false;
	bColdDoorsOpened = false;
	bColdLightsKilled = false;
	bColdEyesShown = false;
	bReplacementFadeStarted = false;
	bHoldingCinematicUntilAbort = false;
	bMergedPhoneWasHidden = false;
	bMergedDiskWasHidden = false;
	MergedMemoryDisk.Reset();
	ColdDoorsOpenedAtSeconds = -1.0f;
	ColdEyesShownAtSeconds = -1.0f;
	SavedLightIntensities.Reset();
	SavedWorldLights.Reset();
	ActiveColdDoors.Reset();

	BeginSceneSetup();

	InteractingController->SetViewTargetWithBlend(this, 0.0f);
	if (InteractingController->PlayerCameraManager)
	{
		InteractingController->PlayerCameraManager->StopCameraFade();
		InteractingController->PlayerCameraManager->StartCameraFade(
			1.0f,
			0.0f,
			0.45f,
			FLinearColor::Black,
			false,
			false);
	}
	bPlaying = true;
	SetActorTickEnabled(true);

	UE_LOG(LogLoop9, Log, TEXT("EndingSceneDirector: playing %d"), static_cast<int32>(EndingType));
	return true;
}

void ALoopEndingSceneDirector::AbortScene()
{
	if (bPlaying || bHoldingCinematicUntilAbort)
	{
		FinishScene(false);
	}
}

void ALoopEndingSceneDirector::BeginSceneSetup()
{
	const FVector PhoneLoc = ResolvePhoneLocation();
	const FVector ArrivalLoc = ResolveArrivalLocation();

	WarmKeyLight->SetVisibility(false);
	WarmKeyLight->SetIntensity(0.0f);
	WarmKeyLight->SetLightColor(FLinearColor(1.0f, 0.82f, 0.55f));
	WarmKeyLight->SetAttenuationRadius(1200.0f);
	AccentLight->SetVisibility(false);
	AccentLight->SetIntensity(0.0f);
	AccentLight->SetLightColor(FLinearColor(0.85f, 0.08f, 0.05f));
	AccentLight->SetAttenuationRadius(900.0f);
	HideColdBetrayalEyes();
	HideMonitorMessage();

	if (ChairActor)
	{
		ChairStartRotation = ChairActor->GetActorRotation();
	}

	switch (ActiveEnding)
	{
	case ELoopEndingType::EscapeTogether:
	{
		CamStartLocation = ArrivalLoc + FVector(0.0f, -40.0f, 55.0f);
		CamStartRotation = (PhoneLoc - CamStartLocation).Rotation();
		CamStartRotation.Pitch = FMath::Clamp(CamStartRotation.Pitch, -12.0f, 8.0f);
		// Shift the cabin framing ~0.5m left so Dragojlo fits on the right,
		// then another ~0.2m further away from him along camera-right.
		{
			const FVector CamRight = FRotationMatrix(CamStartRotation).GetScaledAxis(EAxis::Y);
			CamStartLocation -= CamRight * 70.0f; // 50cm left + 20cm farther from companion
			CamStartRotation = (PhoneLoc - CamStartLocation).Rotation();
			CamStartRotation.Pitch = FMath::Clamp(CamStartRotation.Pitch, -12.0f, 8.0f);
		}
		CamEndLocation = CamStartLocation + CamStartRotation.Vector() * 55.0f;
		CamEndRotation = CamStartRotation;
		WarmKeyLight->SetWorldLocation(ArrivalLoc + FVector(0.0f, 180.0f, 80.0f));
		BeginEscapeTogetherCompanion();
		break;
	}
	case ELoopEndingType::ObedientFool:
	{
		const FVector BaseOffset(95.0f, -70.0f, 28.0f);
		const float BaseDist = BaseOffset.Size();
		const FVector Away = BaseOffset.GetSafeNormal();
		CamStartLocation = PhoneLoc + Away * (BaseDist + ObedientExtraDistanceCm);
		CamStartRotation = (PhoneLoc - CamStartLocation).Rotation();
		const float EndDist = FMath::Max(55.0f, BaseDist * 0.35f + ObedientExtraDistanceCm);
		CamEndLocation = PhoneLoc + Away * EndDist + FVector(0.0f, 0.0f, 4.0f);
		CamEndRotation = (PhoneLoc - CamEndLocation).Rotation();
		MonitorText->SetWorldSize(8.0f);
		MonitorText->SetWorldLocation(PhoneLoc + FVector(0.0f, 0.0f, 12.0f));
		OrientMonitorTextToCamera();
		break;
	}
	case ELoopEndingType::ColdBetrayal:
	{
		CamStartLocation = ArrivalLoc + FVector(0.0f, 20.0f, 52.0f);
		const FVector OutDir = (ArrivalLoc - PhoneLoc).GetSafeNormal2D();
		CamStartRotation = OutDir.IsNearlyZero() ? FRotator(-4.0f, 0.0f, 0.0f) : OutDir.Rotation();
		CamStartRotation.Pitch = -3.0f;
		CamEndLocation = CamStartLocation + CamStartRotation.Vector() * 25.0f;
		CamEndRotation = CamStartRotation;
		BeginColdBetrayalDoors();
		break;
	}
	case ELoopEndingType::ParanoidSurvivor:
	{
		CamStartLocation = ArrivalLoc + FVector(20.0f, -20.0f, 55.0f);
		const FVector ExitLook = (ArrivalLoc - PhoneLoc).GetSafeNormal2D();
		CamStartRotation = ExitLook.Rotation();
		CamStartRotation.Pitch = -3.0f;
		CamEndLocation = CamStartLocation;
		CamEndRotation = CamStartRotation;
		break;
	}
	case ELoopEndingType::MergedMemory:
	{
		CamStartLocation = PhoneLoc + FVector(100.0f, -70.0f, 40.0f);
		CamStartRotation = (PhoneLoc - CamStartLocation).Rotation();
		CamEndLocation = CamStartLocation + CamStartRotation.Vector() * 55.0f;
		CamEndRotation = CamStartRotation;
		AccentLight->SetLightColor(FLinearColor(0.85f, 0.9f, 1.0f));
		AccentLight->SetWorldLocation(PhoneLoc + FVector(0.0f, 0.0f, 120.0f));
		BeginMergedMemoryProps();
		break;
	}
	case ELoopEndingType::TheReplacement:
	default:
	{
		CamStartLocation = PhoneLoc + FVector(110.0f, -75.0f, 36.0f);
		CamStartRotation = (PhoneLoc - CamStartLocation).Rotation();
		CamEndLocation = FMath::Lerp(CamStartLocation, PhoneLoc, 0.94f) + FVector(0.0f, 0.0f, 2.0f);
		CamEndRotation = (PhoneLoc - CamEndLocation).Rotation();
		CamEndRotation.Pitch = FMath::Clamp(CamEndRotation.Pitch, -18.0f, 5.0f);
		break;
	}
	}

	SetCameraPose(CamStartLocation, CamStartRotation);
}

void ALoopEndingSceneDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bPlaying)
	{
		return;
	}

	ElapsedSeconds += DeltaTime;
	UpdateScene(DeltaTime);

	if (ElapsedSeconds >= GetEffectiveSceneDuration())
	{
		FinishScene(true);
	}
}

void ALoopEndingSceneDirector::UpdateScene(float /*DeltaTime*/)
{
	const float T = ElapsedSeconds;
	const float Duration = FMath::Max(GetEffectiveSceneDuration(), 0.1f);

	switch (ActiveEnding)
	{
	case ELoopEndingType::EscapeTogether:
	{
		const float PushAlpha = Smooth01(FMath::GetRangePct(1.0f, 4.0f, T));
		LerpCamera(PushAlpha);

		if (T >= 1.0f)
		{
			WarmKeyLight->SetVisibility(true);
			const float WarmAlpha = Smooth01(FMath::GetRangePct(1.0f, 3.0f, T));
			WarmKeyLight->SetIntensity(FMath::Lerp(0.0f, 3500.0f, WarmAlpha));
		}

		if (T >= 2.85f)
		{
			PlayOneShot(FootstepSound, ResolveArrivalLocation() + FVector(30.0f, 0.0f, 0.0f), AudioBitFootstep1);
		}
		if (T >= 3.5f)
		{
			PlayOneShot(FootstepSound, ResolveArrivalLocation() + FVector(-40.0f, 60.0f, 0.0f), AudioBitFootstep2);
		}
		break;
	}
	case ELoopEndingType::ObedientFool:
	{
		LerpCamera(Smooth01(FMath::GetRangePct(0.6f, 3.8f, T)));
		OrientMonitorTextToCamera();
		if (T >= 0.55f)
		{
			StartObedientPhoneRing();
		}
		if (T >= 1.15f) { ApplyLightDimIndex(0, true); }
		if (T >= 2.0f) { ApplyLightDimIndex(1, true); }
		if (T >= 2.85f) { ApplyLightDimIndex(2, true); }
		if (T >= 2.7f && !bMonitorTextShown)
		{
			ShowMonitorMessage(NSLOCTEXT("Loop9Ending", "TaskComplete", "TASK COMPLETE"));
			OrientMonitorTextToCamera();
		}
		break;
	}
	case ELoopEndingType::ColdBetrayal:
	{
		LerpCamera(Smooth01(FMath::GetRangePct(0.2f, 1.4f, T)));

		if (!bColdDoorsOpened)
		{
			if (AreColdBetrayalDoorsOpen() || T >= ColdDoorOpenTimeoutSeconds)
			{
				bColdDoorsOpened = true;
				ColdDoorsOpenedAtSeconds = T;
				for (const TWeakObjectPtr<ALiftDoorWing>& Door : ActiveColdDoors)
				{
					if (Door.IsValid())
					{
						Door->ForceOpenState();
					}
				}
			}
		}

		if (bColdDoorsOpened
			&& !bColdLightsKilled
			&& T >= ColdDoorsOpenedAtSeconds + ColdBlackoutDelayAfterDoorsSeconds)
		{
			bColdLightsKilled = true;
			KillNearbyWorldLights();
			PlayOneShot(LineCutSound ? LineCutSound.Get() : nullptr, ResolvePhoneLocation(), AudioBitLineCut);
		}

		if (bColdLightsKilled
			&& !bColdEyesShown
			&& T >= ColdDoorsOpenedAtSeconds
				+ ColdBlackoutDelayAfterDoorsSeconds
				+ ColdEyesDelayAfterBlackoutSeconds)
		{
			ShowColdBetrayalEyes();
			bColdEyesShown = true;
			ColdEyesShownAtSeconds = T;
		}
		break;
	}
	case ELoopEndingType::ParanoidSurvivor:
	{
		SetCameraPose(CamStartLocation, CamStartRotation);
		break;
	}
	case ELoopEndingType::MergedMemory:
	{
		LerpCamera(Smooth01(FMath::GetRangePct(1.0f, 3.5f, T)));
		AccentLight->SetVisibility(true);
		if (T >= 1.5f && T < 1.75f)
		{
			PulseAccent(2200.0f);
			PlayOneShot(LightFlickerSound, ResolvePhoneLocation(), AudioBitFlicker1);
		}
		else if (T >= 2.5f && T < 2.75f)
		{
			PulseAccent(2200.0f);
			PlayOneShot(LightFlickerSound, ResolvePhoneLocation(), AudioBitFlicker2);
		}
		else if (T < 1.5f || (T >= 1.75f && T < 2.5f) || T >= 2.75f)
		{
			AccentLight->SetIntensity(400.0f);
		}

		ApplyMergedMemoryVisibility(T);
		break;
	}
	case ELoopEndingType::TheReplacement:
	default:
	{
		// Finish the phone push ~0.4s early; fade to black into the terminal.
		const float PushEnd = FMath::Max(Duration - 0.75f, 3.2f);
		LerpCamera(Smooth01(FMath::GetRangePct(0.4f, PushEnd, T)));
		if (!bReplacementFadeStarted && T >= Duration - 0.95f)
		{
			bReplacementFadeStarted = true;
			if (APlayerController* PC = Controller.Get())
			{
				if (PC->PlayerCameraManager)
				{
					PC->PlayerCameraManager->StartCameraFade(
						0.0f,
						1.0f,
						0.45f,
						FLinearColor::Black,
						false,
						true);
				}
			}
		}
		break;
	}
	}

	if (ActiveEnding != ELoopEndingType::TheReplacement && T >= Duration - 0.4f)
	{
		LerpCamera(1.0f);
	}
}

void ALoopEndingSceneDirector::FinishScene(bool bBroadcastFinished)
{
	if (!bPlaying && !bHoldingCinematicUntilAbort)
	{
		return;
	}

	bPlaying = false;
	SetActorTickEnabled(false);

	if (bBroadcastFinished)
	{
		// Keep cinematic view (Cold Betrayal eyes / blackout) through presenter fade.
		bHoldingCinematicUntilAbort = true;
		OnSceneFinished.Broadcast();
		return;
	}

	bHoldingCinematicUntilAbort = false;
	RestoreWorldMods();

	if (APlayerController* PC = Controller.Get())
	{
		if (AActor* Previous = PreviousViewTarget.Get())
		{
			PC->SetViewTargetWithBlend(Previous, 0.0f);
		}
	}

	Controller.Reset();
	PreviousViewTarget.Reset();
}

void ALoopEndingSceneDirector::RestoreWorldMods()
{
	HideMonitorMessage();
	StopObedientPhoneRing();
	HideColdBetrayalEyes();
	WarmKeyLight->SetVisibility(false);
	WarmKeyLight->SetIntensity(0.0f);
	WarmKeyLight->SetLightColor(FLinearColor(1.0f, 0.82f, 0.55f));
	WarmKeyLight->SetAttenuationRadius(1200.0f);
	AccentLight->SetVisibility(false);
	AccentLight->SetIntensity(0.0f);
	AccentLight->SetLightColor(FLinearColor(0.85f, 0.08f, 0.05f));
	AccentLight->SetAttenuationRadius(900.0f);
	RestoreLightDims();
	RestoreNearbyWorldLights();
	CleanupEscapeTogetherCompanion();
	RestoreMergedMemoryProps();

	for (const TWeakObjectPtr<ALiftDoorWing>& Door : ActiveColdDoors)
	{
		if (Door.IsValid())
		{
			Door->OnMovementFinished.RemoveDynamic(
				this,
				&ALoopEndingSceneDirector::HandleColdDoorMovementFinished);
		}
	}
	ActiveColdDoors.Reset();

	if (ChairActor)
	{
		ChairActor->SetActorRotation(ChairStartRotation);
	}
}

void ALoopEndingSceneDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bPlaying || bHoldingCinematicUntilAbort)
	{
		FinishScene(false);
	}
	Super::EndPlay(EndPlayReason);
}

FVector ALoopEndingSceneDirector::ResolvePhoneLocation() const
{
	return PhoneActor ? PhoneActor->GetActorLocation() : GetActorLocation() + FVector(0.0f, 200.0f, 50.0f);
}

FVector ALoopEndingSceneDirector::ResolveArrivalLocation() const
{
	return ArrivalViewpointActor ? ArrivalViewpointActor->GetActorLocation() : GetActorLocation();
}

FVector ALoopEndingSceneDirector::ResolveChairLocation() const
{
	return ChairActor ? ChairActor->GetActorLocation() : ResolvePhoneLocation() + FVector(40.0f, -80.0f, -40.0f);
}

void ALoopEndingSceneDirector::SetCameraPose(const FVector& Location, const FRotator& Rotation)
{
	SceneCamera->SetWorldLocation(Location);
	SceneCamera->SetWorldRotation(Rotation);
}

void ALoopEndingSceneDirector::LerpCamera(float Alpha)
{
	const float A = Smooth01(Alpha);
	SetCameraPose(
		FMath::Lerp(CamStartLocation, CamEndLocation, A),
		FMath::Lerp(CamStartRotation, CamEndRotation, A));
}

void ALoopEndingSceneDirector::PlayOneShot(USoundBase* Sound, const FVector& Location, uint8 Bit)
{
	if (!Sound || (FiredAudioMask & Bit))
	{
		return;
	}
	FiredAudioMask |= Bit;
	UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
}

void ALoopEndingSceneDirector::ShowMonitorMessage(const FText& Message)
{
	bMonitorTextShown = true;
	MonitorText->SetText(Message);
	MonitorText->SetVisibility(true);
}

void ALoopEndingSceneDirector::HideMonitorMessage()
{
	bMonitorTextShown = false;
	MonitorText->SetVisibility(false);
	MonitorText->SetText(FText::GetEmpty());
}

void ALoopEndingSceneDirector::ApplyLightDimIndex(int32 Index, bool bOff)
{
	if (!DimmableLightActors.IsValidIndex(Index) || !DimmableLightActors[Index])
	{
		return;
	}

	AActor* LightActor = DimmableLightActors[Index];
	TArray<ULightComponent*> Lights;
	LightActor->GetComponents<ULightComponent>(Lights);
	for (ULightComponent* Light : Lights)
	{
		if (!Light)
		{
			continue;
		}

		FSavedLightIntensity* Saved = SavedLightIntensities.FindByPredicate(
			[Light](const FSavedLightIntensity& Entry)
			{
				return Entry.Light.Get() == Light;
			});
		if (bOff)
		{
			if (!Saved)
			{
				FSavedLightIntensity& NewSaved = SavedLightIntensities.AddDefaulted_GetRef();
				NewSaved.Light = Light;
				NewSaved.Intensity = Light->Intensity;
			}
			Light->SetIntensity(0.0f);
		}
		else if (Saved)
		{
			Light->SetIntensity(Saved->Intensity);
		}
	}
}

void ALoopEndingSceneDirector::RestoreLightDims()
{
	for (const FSavedLightIntensity& Saved : SavedLightIntensities)
	{
		if (ULightComponent* Light = Saved.Light.Get())
		{
			Light->SetIntensity(Saved.Intensity);
		}
	}
	SavedLightIntensities.Reset();
}

void ALoopEndingSceneDirector::PulseAccent(float Intensity)
{
	AccentLight->SetVisibility(true);
	AccentLight->SetIntensity(Intensity);
}

void ALoopEndingSceneDirector::OrientMonitorTextToCamera()
{
	if (!MonitorText)
	{
		return;
	}

	const FVector TextLoc = MonitorText->GetComponentLocation();
	const FVector CamLoc = SceneCamera ? SceneCamera->GetComponentLocation() : CamEndLocation;
	FRotator FaceCam = (CamLoc - TextLoc).Rotation();
	FaceCam.Pitch = 0.0f;
	FaceCam.Roll = 0.0f;
	MonitorText->SetWorldRotation(FaceCam);
}

void ALoopEndingSceneDirector::StartObedientPhoneRing()
{
	if (ActivePhoneRingAudio || !PhoneRingSound)
	{
		return;
	}

	ActivePhoneRingAudio = UGameplayStatics::SpawnSoundAtLocation(
		this,
		PhoneRingSound,
		ResolvePhoneLocation(),
		FRotator::ZeroRotator,
		FMath::Clamp(ObedientPhoneRingVolume, 0.0f, 1.0f),
		1.0f,
		0.0f,
		nullptr,
		nullptr,
		true);
	if (ActivePhoneRingAudio)
	{
		ActivePhoneRingAudio->bAutoDestroy = true;
	}
}

void ALoopEndingSceneDirector::StopObedientPhoneRing()
{
	if (IsValid(ActivePhoneRingAudio))
	{
		ActivePhoneRingAudio->Stop();
	}
	ActivePhoneRingAudio = nullptr;
}

void ALoopEndingSceneDirector::GatherColdBetrayalDoors()
{
	ActiveColdDoors.Reset();
	if (ColdBetrayalDoorWings.Num() > 0)
	{
		for (ALiftDoorWing* Door : ColdBetrayalDoorWings)
		{
			if (IsValid(Door))
			{
				ActiveColdDoors.AddUnique(Door);
			}
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ALiftDoorWing> It(World); It; ++It)
		{
			ActiveColdDoors.AddUnique(*It);
		}
	}
}

void ALoopEndingSceneDirector::BeginColdBetrayalDoors()
{
	GatherColdBetrayalDoors();
	bColdDoorsOpened = false;
	ColdDoorsOpenedAtSeconds = -1.0f;

	for (const TWeakObjectPtr<ALiftDoorWing>& DoorPtr : ActiveColdDoors)
	{
		ALiftDoorWing* Door = DoorPtr.Get();
		if (!Door)
		{
			continue;
		}
		Door->OnMovementFinished.RemoveDynamic(
			this,
			&ALoopEndingSceneDirector::HandleColdDoorMovementFinished);
		Door->OnMovementFinished.AddDynamic(this, &ALoopEndingSceneDirector::HandleColdDoorMovementFinished);
		// Ensure a real open beat even if BeginPlay left them open.
		Door->CloseDoorWing();
		Door->OpenDoorWing();
	}
}

void ALoopEndingSceneDirector::HandleColdDoorMovementFinished(bool bIsOpen)
{
	if (!bPlaying || ActiveEnding != ELoopEndingType::ColdBetrayal || !bIsOpen)
	{
		return;
	}

	if (!bColdDoorsOpened && AreColdBetrayalDoorsOpen())
	{
		bColdDoorsOpened = true;
		ColdDoorsOpenedAtSeconds = ElapsedSeconds;
	}
}

bool ALoopEndingSceneDirector::AreColdBetrayalDoorsOpen() const
{
	if (ActiveColdDoors.Num() == 0)
	{
		return true;
	}

	for (const TWeakObjectPtr<ALiftDoorWing>& Door : ActiveColdDoors)
	{
		if (Door.IsValid() && !Door->IsOpened)
		{
			return false;
		}
	}
	return true;
}

void ALoopEndingSceneDirector::KillNearbyWorldLights()
{
	RestoreNearbyWorldLights();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Anchor = ResolveArrivalLocation();
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == this)
		{
			continue;
		}

		TArray<ULightComponent*> Lights;
		Actor->GetComponents<ULightComponent>(Lights);
		for (ULightComponent* Light : Lights)
		{
			if (!Light || FVector::DistSquared(Light->GetComponentLocation(), Anchor) > FMath::Square(4500.0f))
			{
				continue;
			}

			FSavedWorldLight Saved;
			Saved.Light = Light;
			Saved.Intensity = Light->Intensity;
			Saved.bWasVisible = Light->IsVisible();
			SavedWorldLights.Add(Saved);
			Light->SetIntensity(0.0f);
			Light->SetVisibility(false);
		}
	}

	for (int32 i = 0; i < DimmableLightActors.Num(); ++i)
	{
		ApplyLightDimIndex(i, true);
	}
}

void ALoopEndingSceneDirector::RestoreNearbyWorldLights()
{
	for (const FSavedWorldLight& Saved : SavedWorldLights)
	{
		if (ULightComponent* Light = Saved.Light.Get())
		{
			Light->SetIntensity(Saved.Intensity);
			Light->SetVisibility(Saved.bWasVisible);
		}
	}
	SavedWorldLights.Reset();
}

UPointLightComponent* ALoopEndingSceneDirector::EnsureRuntimeEyeLight(
	TObjectPtr<UPointLightComponent>& Slot,
	FName Name)
{
	if (!Slot)
	{
		Slot = NewObject<UPointLightComponent>(this, Name);
		Slot->SetMobility(EComponentMobility::Movable);
		Slot->RegisterComponent();
		Slot->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		Slot->SetUsingAbsoluteLocation(true);
		Slot->SetUsingAbsoluteRotation(true);
		Slot->SetCastShadows(false);
		Slot->SetVisibility(false);
		Slot->SetIntensity(0.0f);
	}
	return Slot;
}

void ALoopEndingSceneDirector::ConfigureColdEyeLight(UPointLightComponent* Light, const FVector& WorldLoc) const
{
	if (!Light)
	{
		return;
	}

	// Close, tiny bloom points — far lights collapse to one screen pixel and look fused.
	constexpr float EyeRadius = 18.0f;
	constexpr float EyeIntensity = 1200.0f;

	Light->SetLightColor(FLinearColor(0.95f, 0.05f, 0.03f));
	Light->SetAttenuationRadius(EyeRadius);
	Light->SetSourceRadius(1.0f);
	Light->SetSoftSourceRadius(0.0f);
	Light->SetSpecularScale(2.5f);
	Light->SetCastShadows(false);
	Light->SetWorldLocation(WorldLoc);
	Light->SetVisibility(true);
	Light->SetIntensity(EyeIntensity);
}

void ALoopEndingSceneDirector::ShowColdBetrayalEyes()
{
	// Use the *current* camera pose (after the cabin push), not CamStart —
	// otherwise eyes sit too far and screen-space separation collapses.
	const FVector CamLoc = SceneCamera ? SceneCamera->GetComponentLocation() : GetActorLocation();
	const FRotator CamRot = SceneCamera ? SceneCamera->GetComponentRotation() : GetActorRotation();
	const FVector Forward = CamRot.Vector().GetSafeNormal();
	const FVector Right = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Y);

	// ~0.75m ahead, ~28cm total span → clearly two dots in FOV.
	constexpr float EyeDist = 75.0f;
	constexpr float EyeHalfSep = 14.0f;
	const FVector EyeBase = CamLoc + Forward * EyeDist + FVector(0.0f, 0.0f, 6.0f);

	// Runtime lights (not AccentLight): existing BP instances may lack EyeLight,
	// which previously left only one glowing point.
	UPointLightComponent* Left = EnsureRuntimeEyeLight(ColdEyeLeft, TEXT("ColdEyeLeft"));
	UPointLightComponent* RightEye = EnsureRuntimeEyeLight(ColdEyeRight, TEXT("ColdEyeRight"));
	ConfigureColdEyeLight(Left, EyeBase - Right * EyeHalfSep);
	ConfigureColdEyeLight(RightEye, EyeBase + Right * EyeHalfSep);

	if (AccentLight)
	{
		AccentLight->SetVisibility(false);
		AccentLight->SetIntensity(0.0f);
	}
	if (EyeLight)
	{
		EyeLight->SetVisibility(false);
		EyeLight->SetIntensity(0.0f);
	}

	UE_LOG(
		LogLoop9,
		Log,
		TEXT("ColdBetrayal eyes: L=%s R=%s dist=%.0f halfSep=%.0f"),
		*EyeBase.ToCompactString(),
		*(EyeBase + Right * EyeHalfSep).ToCompactString(),
		EyeDist,
		EyeHalfSep);
}

void ALoopEndingSceneDirector::HideColdBetrayalEyes()
{
	auto HideOne = [](UPointLightComponent* Light)
	{
		if (Light)
		{
			Light->SetVisibility(false);
			Light->SetIntensity(0.0f);
		}
	};
	HideOne(ColdEyeLeft);
	HideOne(ColdEyeRight);
	HideOne(EyeLight);
}

void ALoopEndingSceneDirector::BeginEscapeTogetherCompanion()
{
	CleanupEscapeTogetherCompanion();

	AActor* Companion = nullptr;
	if (EscapeTogetherCompanionClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Companion = GetWorld()->SpawnActor<ACharacter>(
			EscapeTogetherCompanionClass,
			CamStartLocation,
			CamStartRotation,
			Params);
		bEscapeCompanionSpawned = Companion != nullptr;
	}
	else if (IsValid(EscapeTogetherCompanionActor))
	{
		Companion = EscapeTogetherCompanionActor;
		EscapeCompanionStartTransform = Companion->GetActorTransform();
		bEscapeCompanionWasHidden = Companion->IsHidden();
		bEscapeCompanionCollisionEnabled = Companion->GetActorEnableCollision();
		bEscapeCompanionTickEnabled = Companion->IsActorTickEnabled();
		bEscapeCompanionHasMovementSnapshot = false;
		if (const ACharacter* Character = Cast<ACharacter>(Companion))
		{
			if (const UCharacterMovementComponent* Move = Character->GetCharacterMovement())
			{
				EscapeCompanionMovementMode = static_cast<uint8>(Move->MovementMode);
				EscapeCompanionCustomMovementMode = Move->CustomMovementMode;
				bEscapeCompanionHasMovementSnapshot = true;
			}
		}
		bEscapeCompanionSpawned = false;
	}

	if (!Companion)
	{
		return;
	}

	const FVector Right = FRotationMatrix(CamStartRotation).GetScaledAxis(EAxis::Y);
	const FVector Forward = CamStartRotation.Vector();
	// Camera-right ≈ his right while facing with the player; +0.5m more to his right.
	FVector CompanionLoc = CamStartLocation + Right * 105.0f + Forward * 35.0f;
	CompanionLoc.Z = CamStartLocation.Z - 75.0f; // ~0.3m lower than before
	Companion->SetActorLocation(CompanionLoc);
	// Face mostly with the player, but yaw ~30° left (toward cabin center / player).
	FRotator CompanionRot = CamStartRotation;
	CompanionRot.Yaw -= 30.0f;
	Companion->SetActorRotation(CompanionRot);
	Companion->SetActorHiddenInGame(false);
	Companion->SetActorEnableCollision(false);
	if (ACharacter* AsCharacter = Cast<ACharacter>(Companion))
	{
		AsCharacter->SetActorTickEnabled(false);
		if (UCharacterMovementComponent* Move = AsCharacter->GetCharacterMovement())
		{
			Move->DisableMovement();
		}
	}
	ActiveEscapeCompanion = Companion;
}

void ALoopEndingSceneDirector::CleanupEscapeTogetherCompanion()
{
	if (!ActiveEscapeCompanion.IsValid())
	{
		ActiveEscapeCompanion.Reset();
		bEscapeCompanionSpawned = false;
		bEscapeCompanionWasHidden = false;
		bEscapeCompanionCollisionEnabled = false;
		bEscapeCompanionTickEnabled = false;
		EscapeCompanionMovementMode = 0;
		EscapeCompanionCustomMovementMode = 0;
		bEscapeCompanionHasMovementSnapshot = false;
		return;
	}

	AActor* Companion = ActiveEscapeCompanion.Get();
	if (bEscapeCompanionSpawned)
	{
		Companion->Destroy();
	}
	else if (Companion == EscapeTogetherCompanionActor)
	{
		Companion->SetActorTransform(EscapeCompanionStartTransform);
		Companion->SetActorHiddenInGame(bEscapeCompanionWasHidden);
		Companion->SetActorEnableCollision(bEscapeCompanionCollisionEnabled);
		Companion->SetActorTickEnabled(bEscapeCompanionTickEnabled);
		if (bEscapeCompanionHasMovementSnapshot)
		{
			if (ACharacter* Character = Cast<ACharacter>(Companion))
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					Move->SetMovementMode(
						static_cast<EMovementMode>(EscapeCompanionMovementMode),
						EscapeCompanionCustomMovementMode);
				}
			}
		}
	}

	ActiveEscapeCompanion.Reset();
	bEscapeCompanionSpawned = false;
	bEscapeCompanionWasHidden = false;
	bEscapeCompanionCollisionEnabled = false;
	bEscapeCompanionTickEnabled = false;
	EscapeCompanionMovementMode = 0;
	EscapeCompanionCustomMovementMode = 0;
	bEscapeCompanionHasMovementSnapshot = false;
}

void ALoopEndingSceneDirector::BeginMergedMemoryProps()
{
	RestoreMergedMemoryProps();

	MergedMemoryDisk = MemoryDiskActor ? MemoryDiskActor.Get() : FindNearbyMemoryDisk();
	bMergedPhoneWasHidden = PhoneActor ? PhoneActor->IsHidden() : false;
	bMergedDiskWasHidden = MergedMemoryDisk.IsValid() ? MergedMemoryDisk->IsHidden() : false;

	UE_LOG(LogLoop9, Log, TEXT("MergedMemory desk disk: %s"),
		MergedMemoryDisk.IsValid() ? *MergedMemoryDisk->GetActorNameOrLabel() : TEXT("(none found)"));
}

void ALoopEndingSceneDirector::RestoreMergedMemoryProps()
{
	if (PhoneActor)
	{
		PhoneActor->SetActorHiddenInGame(bMergedPhoneWasHidden);
	}
	if (AActor* Disk = MergedMemoryDisk.Get())
	{
		Disk->SetActorHiddenInGame(bMergedDiskWasHidden);
	}
	MergedMemoryDisk.Reset();
}

void ALoopEndingSceneDirector::ApplyMergedMemoryVisibility(float T)
{
	// Desk props blink in sequence: disk, then phone 0.2s later, then they
	// return the same way, then the disk vanishes for the hold.
	constexpr float DiskHideA = 1.85f;
	constexpr float PhoneHideA = 2.05f;
	constexpr float DiskShow = 2.65f;
	constexpr float PhoneShow = 2.85f;
	constexpr float DiskHideB = 3.55f;

	const bool bPhoneHiddenNow = (T >= PhoneHideA && T < PhoneShow);
	const bool bDiskHiddenNow = (T >= DiskHideA && T < DiskShow) || T >= DiskHideB;

	if (PhoneActor)
	{
		PhoneActor->SetActorHiddenInGame(bPhoneHiddenNow);
	}
	if (AActor* Disk = MergedMemoryDisk.Get())
	{
		Disk->SetActorHiddenInGame(bDiskHiddenNow);
	}
}

AActor* ALoopEndingSceneDirector::FindNearbyMemoryDisk() const
{
	UWorld* World = GetWorld();
	if (!World || !PhoneActor)
	{
		return nullptr;
	}

	const FVector PhoneLoc = PhoneActor->GetActorLocation();
	AActor* Best = nullptr;
	float BestDistSq = FMath::Square(160.0f);

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == PhoneActor || Actor == this)
		{
			continue;
		}

		TArray<UStaticMeshComponent*> Meshes;
		Actor->GetComponents<UStaticMeshComponent>(Meshes);
		bool bLooksLikeDeskMedia = false;
		for (const UStaticMeshComponent* Mesh : Meshes)
		{
			const UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
			if (!Asset)
			{
				continue;
			}

			const FString Name = Asset->GetName();
			if (Name.Contains(TEXT("Floppy"))
				|| Name.Contains(TEXT("Diskette"))
				|| Name.Contains(TEXT("Cassette"))
				|| Name.Contains(TEXT("OfficeTape")))
			{
				bLooksLikeDeskMedia = true;
				break;
			}
		}

		if (!bLooksLikeDeskMedia)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), PhoneLoc);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Actor;
		}
	}

	return Best;
}
