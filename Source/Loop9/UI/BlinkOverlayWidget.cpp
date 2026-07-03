#include "UI/BlinkOverlayWidget.h"
#include "Components/Image.h"

void UBlinkOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetBlinkAlpha(0.0f);
}

void UBlinkOverlayWidget::PlayBlink(float Duration)
{
	BlinkDuration = FMath::Max(0.12f, Duration);
	BlinkHalfDuration = BlinkDuration * 0.5f;
	BlinkElapsed = 0.0f;
	BlinkPhase = EBlinkPhase::Closing;
	bBlinkPlaying = true;
}

void UBlinkOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBlinkPlaying)
	{
		return;
	}

	BlinkElapsed += InDeltaTime;

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
		}
	}
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
