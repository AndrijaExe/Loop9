#include "UI/SignalBurstWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

namespace
{
	constexpr float ScrambleIntervalSeconds = 0.045f;
}

TSharedRef<SWidget> USignalBurstWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BurstCanvas"));
		WidgetTree->RootWidget = Canvas;

		BaseImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BurstBase"));
		BaseImage->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		if (UCanvasPanelSlot* BaseSlot = Canvas->AddChildToCanvas(BaseImage))
		{
			BaseSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BaseSlot->SetOffsets(FMargin(0.0f));
		}

		Bars.Reset();
		const int32 Count = FMath::Clamp(BarCount, 4, 64);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			UImage* Bar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("BurstBar%d"), Index));
			Bar->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			if (UCanvasPanelSlot* BarSlot = Canvas->AddChildToCanvas(Bar))
			{
				BarSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
				BarSlot->SetOffsets(FMargin(0.0f));
			}
			Bars.Add(Bar);
		}
	}

	TSharedRef<SWidget> Built = Super::RebuildWidget();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Built;
}

void USignalBurstWidget::NativeDestruct()
{
	StopTicker();
	Super::NativeDestruct();
}

void USignalBurstWidget::Play(float Duration)
{
	BurstDuration = FMath::Max(0.15f, Duration);
	Elapsed = 0.0f;
	SinceScramble = ScrambleIntervalSeconds;
	bPlaying = true;

	StopTicker();
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
		{
			return TickBurst(DeltaTime);
		}));
}

void USignalBurstWidget::StopTicker()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	TickerHandle.Reset();
}

bool USignalBurstWidget::TickBurst(float DeltaTime)
{
	if (!bPlaying)
	{
		TickerHandle.Reset();
		return false;
	}

	Elapsed += FMath::Max(0.0f, DeltaTime);
	SinceScramble += FMath::Max(0.0f, DeltaTime);

	if (Elapsed >= BurstDuration)
	{
		bPlaying = false;
		Clear();
		TickerHandle.Reset();
		RemoveFromParent();
		return false;
	}

	if (SinceScramble >= ScrambleIntervalSeconds)
	{
		SinceScramble = 0.0f;
		Scramble();
	}

	return true;
}

void USignalBurstWidget::Scramble()
{
	// Strongest in the middle of the burst, thinner at both ends, so it reads
	// as a signal dropping and coming back rather than a switch.
	const float T = BurstDuration > 0.0f ? Elapsed / BurstDuration : 1.0f;
	const float Envelope = FMath::Sin(T * PI);

	if (BaseImage)
	{
		// Mostly black with the odd bright frame.
		const bool bWhiteFrame = FMath::FRand() < 0.12f;
		const float Grey = bWhiteFrame ? FMath::FRandRange(0.55f, 0.85f) : FMath::FRandRange(0.0f, 0.08f);
		const float Alpha = FMath::Clamp(FMath::FRandRange(0.35f, 0.95f) * Envelope + 0.15f, 0.0f, 1.0f);
		BaseImage->SetColorAndOpacity(FLinearColor(Grey, Grey, Grey, Alpha));
	}

	const int32 Count = Bars.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UImage* Bar = Bars[Index];
		if (!Bar)
		{
			continue;
		}

		UCanvasPanelSlot* BarSlot = Cast<UCanvasPanelSlot>(Bar->Slot);
		if (!BarSlot)
		{
			continue;
		}

		// Bars keep a rough vertical order with a jump each frame, like a
		// picture that has lost sync.
		const float Band = 1.0f / static_cast<float>(Count);
		const float Y0 = FMath::Clamp(Index * Band + FMath::FRandRange(-Band, Band), 0.0f, 1.0f);
		const float Height = FMath::FRandRange(Band * 0.15f, Band * 1.1f);
		const float Y1 = FMath::Clamp(Y0 + Height, 0.0f, 1.0f);
		BarSlot->SetAnchors(FAnchors(0.0f, Y0, 1.0f, Y1));
		BarSlot->SetOffsets(FMargin(0.0f));

		// A horizontal tear: some bars sit slightly off to one side.
		const float Shift = FMath::FRand() < 0.3f ? FMath::FRandRange(-0.08f, 0.08f) : 0.0f;
		BarSlot->SetAlignment(FVector2D(Shift, 0.0f));

		const bool bLit = FMath::FRand() < 0.55f;
		const float Grey = bLit ? FMath::FRandRange(0.25f, 0.9f) : FMath::FRandRange(0.0f, 0.06f);
		const float Alpha = bLit ? FMath::FRandRange(0.25f, 0.8f) * Envelope : FMath::FRandRange(0.4f, 0.9f) * Envelope;
		Bar->SetColorAndOpacity(FLinearColor(Grey, Grey, Grey, FMath::Clamp(Alpha, 0.0f, 1.0f)));
	}
}

void USignalBurstWidget::Clear()
{
	if (BaseImage)
	{
		BaseImage->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	}
	for (UImage* Bar : Bars)
	{
		if (Bar)
		{
			Bar->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}
