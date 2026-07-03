#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop/LoopTypes.h"
#include "LoopEndingPresenterSubsystem.generated.h"

class URelationshipSubsystem;
class UEndingWidget;
class UReplacementTerminalWidget;

UCLASS()
class LOOP9_API ULoopEndingPresenterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void TriggerEndingSequence(URelationshipSubsystem* Relationship);
	void ReturnToMainMenu(APlayerController* PlayerController);

private:
	void ShowEndingWidget(ELoopEndingType EndingType, int32 TotalResets, int32 TotalAIInteractions);
	void ShowReplacementTerminal();
	TSubclassOf<UEndingWidget> ResolveEndingWidgetClass(ELoopEndingType EndingType) const;

	UFUNCTION()
	void HandleEndingContinueRequested();

	UFUNCTION()
	void HandleReplacementTerminalContinueRequested();

	UPROPERTY(Transient)
	TObjectPtr<UEndingWidget> ActiveEndingWidget;

	UPROPERTY(Transient)
	TObjectPtr<UReplacementTerminalWidget> ActiveTerminalWidget;
};
