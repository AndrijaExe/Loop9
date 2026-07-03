// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterGameMode.h"
#include "ShooterUI.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!ShooterUIClass)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PlayerController)
	{
		return;
	}

	ShooterUI = CreateWidget<UShooterUI>(PlayerController, ShooterUIClass);
	if (ShooterUI)
	{
		ShooterUI->AddToViewport(0);
	}
}

void AShooterGameMode::IncrementTeamScore(uint8 TeamByte)
{
	// retrieve the team score if any
	int32 Score = 0;
	if (int32* FoundScore = TeamScores.Find(TeamByte))
	{
		Score = *FoundScore;
	}

	// increment the score for the given team
	++Score;
	TeamScores.Add(TeamByte, Score);

	// update the UI
	if (ShooterUI)
	{
		ShooterUI->BP_UpdateScore(TeamByte, Score);
	}
}
