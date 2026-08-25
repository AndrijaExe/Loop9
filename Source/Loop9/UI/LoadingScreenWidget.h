// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingScreenWidget.generated.h"

/**
 * Loading Screen Widget - Shows while level is loading
 */
UCLASS()
class LOOP9_API ULoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	/** Get a random loading tip */
	UFUNCTION(BlueprintCallable, Category = "Loading")
	FText GetRandomLoadingTip() const;

	/** Current loading tip (bind to Text widget in Blueprint) */
	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	FText CurrentLoadingTip;

	/** Start fade out animation (call from Blueprint or C++) */
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void StartFadeOut();

	/** Called when fade out completes (implement in Blueprint if needed) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loading")
	void OnFadeOutComplete();

	/** Fade out duration in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	float FadeOutDuration = 0.5f;

	/** Root canvas panel (bind from Blueprint) */
	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* RootCanvas;

private:
	/** Hardcoded loading tips */
	static const TArray<FText> LoadingTips;

	/** Fade out state */
	bool bIsFadingOut = false;
	float FadeOutProgress = 0.0f;
};
