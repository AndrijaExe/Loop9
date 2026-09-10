#include "LoopNumberSign.h"
#include "Anomaly/LoopNumberAnomalyComponent.h"
#include "Components/TextRenderComponent.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

ALoopNumberSign::ALoopNumberSign()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.05f;

	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
	RootComponent = TextRender;

	TextRender->SetHorizontalAlignment(EHTA_Center);
	TextRender->SetVerticalAlignment(EVRTA_TextCenter);
	TextRender->SetWorldSize(60.0f);
	TextRender->SetTextRenderColor(TextColor);
	TextRender->SetText(FText::FromString(TEXT("LOOP 1")));

	LoopNumberAnomaly = CreateDefaultSubobject<ULoopNumberAnomalyComponent>(TEXT("LoopNumberAnomaly"));
}

void ALoopNumberSign::BeginPlay()
{
	Super::BeginPlay();
	TextRender->SetTextRenderColor(TextColor);
	ConfigurePresentationTick();
	RefreshLoopText();
}

void ALoopNumberSign::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(IdleRefreshTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ALoopNumberSign::SetAnomalyGlitchActive(bool bActive)
{
	if (bAnomalyGlitchActive == bActive)
	{
		return;
	}

	bAnomalyGlitchActive = bActive;
	if (!bAnomalyGlitchActive)
	{
		bGlitchActive = false;
		bFlickerDropout = false;
		GlitchAccumulator = 0.0f;
		GlitchElapsed = 0.0f;
	}

	ConfigurePresentationTick();
	RefreshLoopText();
}

void ALoopNumberSign::ConfigurePresentationTick()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IdleRefreshTimerHandle);
	}

	const bool bNeedsContinuousFx = WantsPresentationFx();
	PrimaryActorTick.TickInterval = bNeedsContinuousFx ? 0.05f : FMath::Max(0.1f, UpdateInterval);
	SetActorTickEnabled(bNeedsContinuousFx);

	if (!bNeedsContinuousFx && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			IdleRefreshTimerHandle,
			this,
			&ALoopNumberSign::RefreshLoopText,
			FMath::Max(0.1f, UpdateInterval),
			true);
	}
}

void ALoopNumberSign::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RunningTime += DeltaTime;

	if (!WantsPresentationFx())
	{
		// A Sequencer key just cleared the cosmetic flags: back to idle refresh.
		bGlitchActive = false;
		bFlickerDropout = false;
		ConfigurePresentationTick();
		RefreshLoopText();
		return;
	}

	if (bAnomalyGlitchActive || bEnableGlitch)
	{
		if (!bGlitchActive)
		{
			GlitchAccumulator += DeltaTime;
			if (GlitchAccumulator >= GlitchInterval)
			{
				bGlitchActive = true;
				GlitchElapsed = 0.0f;
				GlitchAccumulator = 0.0f;
			}
		}
		else
		{
			GlitchElapsed += DeltaTime;
			if (GlitchElapsed >= GlitchDuration)
			{
				bGlitchActive = false;
				GlitchElapsed = 0.0f;
			}
		}
	}
	else
	{
		bGlitchActive = false;
	}

	const bool bFlicker = bAnomalyGlitchActive || bEnableFlicker;
	bFlickerDropout = bFlicker && FMath::FRand() < (FlickerDropoutChancePerSecond * DeltaTime);

	UpdateAccumulator += DeltaTime;
	if (UpdateAccumulator >= UpdateInterval)
	{
		UpdateAccumulator = 0.0f;
		RefreshLoopText();
	}
	else
	{
		UpdateDynamicColor(LastShownLoop);
	}
}

void ALoopNumberSign::RefreshLoopText()
{
	if (!GetGameInstance())
	{
		return;
	}

	ULoopManagerSubsystem* LoopManager = GetGameInstance()->GetSubsystem<ULoopManagerSubsystem>();
	if (!LoopManager)
	{
		return;
	}

	// A Sequencer key can flip a cosmetic flag without going through
	// SetAnomalyGlitchActive; the idle refresh is where that gets noticed.
	if (!IsActorTickEnabled() && WantsPresentationFx())
	{
		ConfigurePresentationTick();
	}

	LastShownLoop = FixedLoopValue > 0 ? FixedLoopValue : LoopManager->CurrentLoop;

	if (bGlitchActive)
	{
		TextRender->SetText(FText::FromString(BuildGlitchText(LastShownLoop)));
	}
	else
	{
		TextRender->SetText(FText::FromString(FString::Printf(TEXT("%s %d"), *Prefix, LastShownLoop)));
	}

	UpdateDynamicColor(LastShownLoop);
}

void ALoopNumberSign::UpdateDynamicColor(int32 LoopValue)
{
	const float Progress = FMath::Clamp((LoopValue - 1) / static_cast<float>(FMath::Max(1, MaxLoopForFullIntensity - 1)), 0.0f, 1.0f);
	const FLinearColor Base = FLinearColor(TextColor);
	const FLinearColor Aggressive = FLinearColor(AggressiveTextColor);
	FLinearColor Result = FMath::Lerp(Base, Aggressive, Progress);

	if (bAnomalyGlitchActive || bEnableFlicker)
	{
		const float FlickerWave = (FMath::Sin(RunningTime * FlickerSpeed) * 0.5f) + 0.5f;
		const float FlickerMultiplier = 1.0f - (FlickerStrength * FlickerWave);
		Result *= FlickerMultiplier;
	}

	if (bFlickerDropout)
	{
		Result *= 0.25f;
	}

	Result.A = 1.0f;
	TextRender->SetTextRenderColor(Result.ToFColor(true));
}

FString ALoopNumberSign::BuildGlitchText(int32 LoopValue) const
{
	(void)LoopValue;
	const int32 GlitchPattern = FMath::RandRange(0, 2);
	if (GlitchPattern == 0)
	{
		return FString::Printf(TEXT("%s ?"), *Prefix);
	}
	if (GlitchPattern == 1)
	{
		return TEXT("L00P ?");
	}
	return FString::Printf(TEXT("%s ??"), *Prefix);
}

