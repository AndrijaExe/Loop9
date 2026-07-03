#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionPromptProvider.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractionPromptProvider : public UInterface
{
	GENERATED_BODY()
};

class LOOP9_API IInteractionPromptProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void SetInteractionPrompt(const FText& PromptText, bool bVisible);
};
