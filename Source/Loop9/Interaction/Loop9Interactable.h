#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Loop9Interactable.generated.h"

class APlayerController;

UINTERFACE(BlueprintType)
class LOOP9_API ULoop9Interactable : public UInterface
{
	GENERATED_BODY()
};

class LOOP9_API ILoop9Interactable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool TryInteract(APlayerController* InteractingController);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPromptText() const;
};
