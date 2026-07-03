#include "LoopNumberSign.h"
#include "Components/TextRenderComponent.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "Engine/GameInstance.h"

ALoopNumberSign::ALoopNumberSign()
{
	PrimaryActorTick.bCanEverTick = true;

	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
	RootComponent = TextRender;

	TextRender->SetHorizontalAlignment(EHTA_Center);
	TextRender->SetVerticalAlignment(EVRTA_TextCenter);
	TextRender->SetWorldSize(60.0f);
	TextRender->SetTextRenderColor(TextColor);
	TextRender->SetText(FText::FromString(TEXT("LOOP 1")));
}

void ALoopNumberSign::BeginPlay()
{
	Super::BeginPlay();
	TextRender->SetTextRenderColor(TextColor);
	RefreshLoopText();
}

void ALoopNumberSign::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RunningTime += DeltaTime;

	if (!GetGameInstance())
	{
		return;
	}

	ULoopManagerSubsystem* LoopManager = GetGameInstance()->GetSubsystem<ULoopManagerSubsystem>();
	if (!LoopManager)
	{
		return;
	}

	const int32 CurrentLoopValue = LoopManager->CurrentLoop;

	if (bEnableGlitch && GlitchEveryNLoops > 0 && CurrentLoopValue > 0 && (CurrentLoopValue % GlitchEveryNLoops == 0))
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
		GlitchAccumulator = 0.0f;
	}

	if (bEnableFlicker)
	{
		bFlickerDropout = FMath::FRand() < (FlickerDropoutChancePerSecond * DeltaTime);
	}
	else
	{
		bFlickerDropout = false;
	}

	UpdateAccumulator += DeltaTime;
	if (UpdateAccumulator >= UpdateInterval)
	{
		UpdateAccumulator = 0.0f;
		RefreshLoopText();
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

  LastShownLoop = LoopManager->CurrentLoop;

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

	if (bEnableFlicker)
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
	const int32 GlitchPattern = FMath::RandRange(0, 2);
	if (GlitchPattern == 0)
	{
		return FString::Printf(TEXT("L00P ?"));
	}
	if (GlitchPattern == 1)
	{
		return FString::Printf(TEXT("%s ??"), *Prefix);
	}
	return FString::Printf(TEXT("%s %d"), *Prefix, FMath::Max(0, LoopValue + FMath::RandRange(-1, 1)));
}
