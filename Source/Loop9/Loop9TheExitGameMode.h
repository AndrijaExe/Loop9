#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Loop9TheExitGameMode.generated.h"

class ULevelSequence;

/**
 * GameMode for the 1.1 secret-ending level (the apartment). The office level
 * fades to black and opens this map; ULoopEndingPresenterSubsystem survives the
 * travel and, once this GameMode begins play, holds the black, plays
 * ExitSequence, then shows the THE EXIT card and returns to the main menu.
 *
 * No pawn, no gameplay: the player is a spectator whose view the sequence's
 * Camera Cut track owns. Set it as the apartment map's GameMode Override.
 */
UCLASS()
class LOOP9_API ALoop9TheExitGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALoop9TheExitGameMode();

	/** The cutscene authored for this map. Empty = hold black briefly, then the card. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cinematics")
	TSoftObjectPtr<ULevelSequence> ExitSequence;

protected:
	virtual void BeginPlay() override;
};
