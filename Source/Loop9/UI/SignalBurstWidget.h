#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Blueprint/UserWidget.h"
#include "SignalBurstWidget.generated.h"

class UCanvasPanel;
class UImage;

/**
 * 1.1: a short "bad signal" burst over the whole screen — black frames, grey
 * scan bars that jump, a few bright tears — built entirely in code so it needs
 * no texture and no Blueprint. Used when the player walks into the Watcher,
 * so they never get a clean look at how he leaves.
 *
 * Create it through ALoop9PlayerController::PlaySignalBurst. It removes itself
 * from the viewport when the burst ends.
 */
UCLASS()
class LOOP9_API USignalBurstWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Starts the burst. Duration is clamped to at least 0.15 s. */
	UFUNCTION(BlueprintCallable, Category = "Signal Burst")
	void Play(float Duration = 0.6f);

	UFUNCTION(BlueprintPure, Category = "Signal Burst")
	bool IsPlaying() const { return bPlaying; }

	/** How many horizontal bars make up the static. Set before Play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Signal Burst", meta = (ClampMin = "4", ClampMax = "64"))
	int32 BarCount = 22;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	bool TickBurst(float DeltaTime);
	void StopTicker();
	void Scramble();
	void Clear();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Canvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BaseImage = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> Bars;

	bool bPlaying = false;
	float BurstDuration = 0.6f;
	float Elapsed = 0.0f;
	float SinceScramble = 0.0f;
	FTSTicker::FDelegateHandle TickerHandle;
};
