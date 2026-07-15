// Fill out your copyright notice in the Description page of Project Settings.

#include "LoadingScreenWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"

#define LOCTEXT_NAMESPACE "Loop9Loading"

// Hardcoded loading tips (6-7 tips)
const TArray<FText> ULoadingScreenWidget::LoadingTips = {
	LOCTEXT("Tip01", "Look for objects that disappear..."),
	LOCTEXT("Tip02", "Pay close attention to your surroundings"),
	LOCTEXT("Tip03", "Trust your instincts, but verify everything"),
	LOCTEXT("Tip04", "Not everything is as it seems"),
	LOCTEXT("Tip05", "The office changes when you're not looking"),
	LOCTEXT("Tip06", "Dragojlo has been here for a very long time"),
	LOCTEXT("Tip07", "Every loop is a chance to escape")
};

void ULoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Set random loading tip on construct
	CurrentLoadingTip = GetRandomLoadingTip();

	// Reset fade state
	bIsFadingOut = false;
	FadeOutProgress = 0.0f;

	// Ensure RootCanvas is at full opacity
	if (RootCanvas)
	{
		RootCanvas->SetRenderOpacity(1.0f);
		UE_LOG(LogTemp, Log, TEXT("LoadingScreenWidget: RootCanvas found and set to full opacity"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadingScreenWidget: RootCanvas is NULL! Make sure Canvas Panel is named 'RootCanvas' in Blueprint"));
	}

	UE_LOG(LogTemp, Log, TEXT("LoadingScreenWidget: Constructed with tip: %s"), *CurrentLoadingTip.ToString());
}

void ULoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Handle fade out
	if (bIsFadingOut && RootCanvas)
	{
		FadeOutProgress += InDeltaTime;

		UE_LOG(LogTemp, VeryVerbose, TEXT("LoadingScreenWidget: Fading... Progress: %.2f / %.2f"), FadeOutProgress, FadeOutDuration);

		if (FadeOutProgress >= FadeOutDuration)
		{
			// Fade complete
			bIsFadingOut = false;
			RootCanvas->SetRenderOpacity(0.0f);

			UE_LOG(LogTemp, Log, TEXT("LoadingScreenWidget: Fade out complete!"));

			// Call Blueprint event
			OnFadeOutComplete();

			// Remove from viewport
			RemoveFromParent();

			UE_LOG(LogTemp, Log, TEXT("LoadingScreenWidget: Removed from viewport"));
		}
		else
		{
			// Calculate fade alpha (1.0 → 0.0)
			float t = FadeOutProgress / FadeOutDuration;
			float Alpha = 1.0f - t;

			// Apply opacity to root canvas
			RootCanvas->SetRenderOpacity(Alpha);

			UE_LOG(LogTemp, Verbose, TEXT("LoadingScreenWidget: Opacity set to %.2f"), Alpha);
		}
	}
}

void ULoadingScreenWidget::StartFadeOut()
{
	if (!bIsFadingOut)
	{
		bIsFadingOut = true;
		FadeOutProgress = 0.0f;
		UE_LOG(LogTemp, Log, TEXT("LoadingScreenWidget: Starting fade out"));
	}
}

FText ULoadingScreenWidget::GetRandomLoadingTip() const
{
	if (LoadingTips.Num() == 0)
	{
		return LOCTEXT("LoadingFallback", "Loading...");
	}

	// Get random tip
	int32 RandomIndex = FMath::RandRange(0, LoadingTips.Num() - 1);
	return LoadingTips[RandomIndex];
}

void ULoadingScreenWidget::ShowLoadingScreen(UObject* WorldContextObject)
{
	UE_LOG(LogTemp, Log, TEXT("LoadingScreenWidget: Show (simple version)"));
}

void ULoadingScreenWidget::HideLoadingScreen(UObject* WorldContextObject)
{
	UE_LOG(LogTemp, Log, TEXT("LoadingScreenWidget: Hide"));
}

#undef LOCTEXT_NAMESPACE
