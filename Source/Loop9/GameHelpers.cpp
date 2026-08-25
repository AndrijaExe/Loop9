// Fill out your copyright notice in the Description page of Project Settings.

#include "GameHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/CoreDelegates.h"
#include "MoviePlayer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SThrobber.h"

#define LOCTEXT_NAMESPACE "Loop9Loading"

namespace
{
	TStrongObjectPtr<UUserWidget> GLoadingUMGWidget;
	FDelegateHandle GMoviePlaybackFinishedHandle;
	FDelegateHandle GTravelFailureHandle;
	FDelegateHandle GPreExitHandle;

	void ReleaseLoadingUMGWidget()
	{
		GLoadingUMGWidget.Reset();
	}

	void HandleLoadingMovieFinished()
	{
		ReleaseLoadingUMGWidget();
	}

	void HandleTravelFailure(UWorld*, ETravelFailure::Type, const FString&)
	{
		ReleaseLoadingUMGWidget();
	}

	void HandlePreExit()
	{
		ReleaseLoadingUMGWidget();

		if (GEngine && GTravelFailureHandle.IsValid())
		{
			GEngine->OnTravelFailure().Remove(GTravelFailureHandle);
			GTravelFailureHandle.Reset();
		}

		if (IsMoviePlayerEnabled() && GMoviePlaybackFinishedHandle.IsValid())
		{
			GetMoviePlayer()->OnMoviePlaybackFinished().Remove(GMoviePlaybackFinishedHandle);
			GMoviePlaybackFinishedHandle.Reset();
		}
	}

	void EnsureLoadingLifecycleDelegates()
	{
		if (!GMoviePlaybackFinishedHandle.IsValid())
		{
			GMoviePlaybackFinishedHandle =
				GetMoviePlayer()->OnMoviePlaybackFinished().AddStatic(&HandleLoadingMovieFinished);
		}

		if (GEngine && !GTravelFailureHandle.IsValid())
		{
			GTravelFailureHandle = GEngine->OnTravelFailure().AddStatic(&HandleTravelFailure);
		}

		if (!GPreExitHandle.IsValid())
		{
			GPreExitHandle = FCoreDelegates::OnPreExit.AddStatic(&HandlePreExit);
		}
	}

	TSharedRef<SWidget> BuildLoadingSlateWidget()
	{
		// Keep keys in sync with ULoadingScreenWidget::LoadingTips so translators see one set.
		static const TArray<FText> Tips = {
			LOCTEXT("Tip01", "Look for objects that disappear..."),
			LOCTEXT("Tip02", "Pay close attention to your surroundings"),
			LOCTEXT("Tip03", "Trust your instincts, but verify everything"),
			LOCTEXT("Tip04", "Not everything is as it seems"),
			LOCTEXT("Tip05", "The office changes when you're not looking"),
			LOCTEXT("Tip06", "Dragojlo has been here for a very long time"),
			LOCTEXT("Tip07", "Every loop is a chance to escape")
		};

		const FText& RandomTip = Tips[FMath::RandRange(0, Tips.Num() - 1)];

		return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor::Black)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 20)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LoadingHeader", "LOADING..."))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 48))
				.ColorAndOpacity(FLinearColor::White)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 24)
			[
				SNew(SThrobber)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(30, 0)
			[
				SNew(STextBlock)
				.Text(RandomTip)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
				.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
				.Justification(ETextJustify::Center)
			]
		];
	}

	void SetupBlockingLoadingScreen(UWorld* World, TSubclassOf<UUserWidget> LoadingScreenClass, float MinimumDisplayTime)
	{
		if (!IsMoviePlayerEnabled())
		{
			return;
		}

		EnsureLoadingLifecycleDelegates();
		ReleaseLoadingUMGWidget();

		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
		LoadingScreen.bMoviesAreSkippable = false;
		LoadingScreen.bWaitForManualStop = false;
		LoadingScreen.MinimumLoadingScreenDisplayTime = MinimumDisplayTime;

		// Preferred: use user-selected UMG widget blueprint
		if (World && LoadingScreenClass)
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
			{
				UUserWidget* LoadingWidget = CreateWidget<UUserWidget>(PC, LoadingScreenClass);
				if (LoadingWidget)
				{
					GLoadingUMGWidget.Reset(LoadingWidget);
					LoadingScreen.WidgetLoadingScreen = LoadingWidget->TakeWidget();
				}
			}
		}

		// Fallback: built-in Slate loading screen
		if (!LoadingScreen.WidgetLoadingScreen.IsValid())
		{
			LoadingScreen.WidgetLoadingScreen = BuildLoadingSlateWidget();
		}

		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}
}

void UGameHelpers::LoadLevelWithLoadingScreen(
	UObject* WorldContextObject, 
	FName LevelName,
	TSubclassOf<UUserWidget> LoadingScreenClass)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameHelpers: Invalid world context"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("GameHelpers: Loading level '%s'"), *LevelName.ToString());

	if (LoadingScreenClass)
	{
		SetupBlockingLoadingScreen(World, LoadingScreenClass, 0.0f);
	}

	UGameplayStatics::OpenLevel(World, LevelName);
}

#undef LOCTEXT_NAMESPACE
