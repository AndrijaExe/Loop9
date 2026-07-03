#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BlinkOverlayWidget.generated.h"

class UImage;

UCLASS()
class LOOP9_API UBlinkOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Blink")
	void PlayBlink(float Duration = 0.35f);

	UFUNCTION(BlueprintCallable, Category = "Blink")
	bool IsBlinkPlaying() const { return bBlinkPlaying; }

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* BlinkImage;

private:
	enum class EBlinkPhase : uint8
	{
		None,
		Closing,
		Opening
	};

	void SetBlinkAlpha(float Alpha);

	bool bBlinkPlaying = false;
	EBlinkPhase BlinkPhase = EBlinkPhase::None;
	float BlinkDuration = 0.35f;
	float BlinkHalfDuration = 0.175f;
	float BlinkElapsed = 0.0f;
};
