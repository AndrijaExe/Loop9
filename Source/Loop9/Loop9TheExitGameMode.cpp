#include "Loop9TheExitGameMode.h"

#include "Engine/GameInstance.h"
#include "GameFramework/SpectatorPawn.h"
#include "Subsystems/LoopEndingPresenterSubsystem.h"

ALoop9TheExitGameMode::ALoop9TheExitGameMode()
{
	// Nobody walks here; the sequence camera is the only viewpoint.
	DefaultPawnClass = nullptr;
	SpectatorClass = ASpectatorPawn::StaticClass();
	bStartPlayersAsSpectators = true;
}

void ALoop9TheExitGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULoopEndingPresenterSubsystem* Presenter = GameInstance->GetSubsystem<ULoopEndingPresenterSubsystem>())
		{
			Presenter->ContinueTheExitInLevel(this);
		}
	}
}
