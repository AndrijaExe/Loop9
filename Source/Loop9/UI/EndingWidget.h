#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LoopManagerSubsystem.h"
#include "EndingWidget.generated.h"

UCLASS()
class LOOP9_API UEndingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Ending")
	void InitializeEnding(ELoopEndingType EndingType, int32 InResets, int32 InAIInteractions);

	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	FText EndingTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	FText EndingDescription;

	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	FText EndingStats;
};
