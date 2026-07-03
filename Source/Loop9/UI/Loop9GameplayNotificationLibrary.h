#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loop9GameplayNotificationLibrary.generated.h"

UCLASS()
class LOOP9_API ULoop9GameplayNotificationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications", meta = (WorldContext = "WorldContextObject", DisplayName = "Add Message"))
	static void AddGameplayMessage(UObject* WorldContextObject, const FText& Message, float TimeDuration = 4.0f);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications", meta = (WorldContext = "WorldContextObject", DisplayName = "Add Message (String)"))
	static void AddGameplayMessageString(UObject* WorldContextObject, const FString& Message, float TimeDuration = 4.0f);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications", meta = (WorldContext = "WorldContextObject", DeprecatedFunction, DeprecationMessage = "Use Add Message instead"))
	static void ShowGameplayNotification(UObject* WorldContextObject, const FText& Message, float DisplayDuration = 4.0f);
};
