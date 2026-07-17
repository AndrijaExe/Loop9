#include "UI/BlinkOverlayWidget.h"
#include "Components/Image.h"

void UBlinkOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetBlinkAlpha(0.0f);
}

void UBlinkOverlayWidget::NativeDestruct()
{
	StopBlinkTicker();
	Super::NativeDestruct();
}

void UBlinkOverlayWidget::PlayBlink(float Duration)
{
	BlinkDuration = FMath::Max(0.12f, Duration);
	BlinkHalfDuration = BlinkDuration * 0.5f;
	BlinkElapsed = 0.0f;
	BlinkPhase = EBlinkPhase::Closing;
	bBlinkPlaying = true;

	StopBlinkTicker();
	BlinkTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
		{
			return TickBlink(DeltaTime);
		}));
}

void UBlinkOverlayWidget::StopBlinkTicker()
{
	FTSTicker::GetCoreTicker().RemoveTicker(BlinkTickerHandle);
	BlinkTickerHandle.Reset();
}

bool UBlinkOverlayWidget::TickBlink(float DeltaTime)
{
	if (!bBlinkPlaying)
	{
		BlinkTickerHandle.Reset();
		return false;
	}

	BlinkElapsed += FMath::Max(0.0f, DeltaTime);

	if (BlinkPhase == EBlinkPhase::Closing)
	{
		const float T = FMath::Clamp(BlinkElapsed / BlinkHalfDuration, 0.0f, 1.0f);
		SetBlinkAlpha(T);

		if (T >= 1.0f)
		{
			BlinkPhase = EBlinkPhase::Opening;
			BlinkElapsed = 0.0f;
		}
	}
	else if (BlinkPhase == EBlinkPhase::Opening)
	{
		const float T = FMath::Clamp(BlinkElapsed / BlinkHalfDuration, 0.0f, 1.0f);
		SetBlinkAlpha(1.0f - T);

		if (T >= 1.0f)
		{
			bBlinkPlaying = false;
			BlinkPhase = EBlinkPhase::None;
			SetBlinkAlpha(0.0f);
			BlinkTickerHandle.Reset();
			return false;
		}
	}

	return true;
}

void UBlinkOverlayWidget::SetBlinkAlpha(float Alpha)
{
	if (BlinkImage)
	{
		FLinearColor C = BlinkImage->GetColorAndOpacity();
		C.A = Alpha;
		BlinkImage->SetColorAndOpacity(C);
	}
	else
	{
		SetRenderOpacity(Alpha);
	}
}
