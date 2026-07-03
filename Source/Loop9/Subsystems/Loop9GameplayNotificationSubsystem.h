#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Loop9GameplayNotificationSubsystem.generated.h"

class ULoop9NotificationWidget;

UCLASS()
class LOOP9_API ULoop9GameplayNotificationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications", meta = (WorldContext = "WorldContextObject"))
	static ULoop9GameplayNotificationSubsystem* GetGameplayNotifications(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications")
	void AddMessage(const FText& Message, float TimeDuration = 4.0f);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications")
	void AddMessageString(const FString& Message, float TimeDuration = 4.0f);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications")
	void ClearMessages();

	UFUNCTION(BlueprintPure, Category = "Gameplay Notifications")
	ULoop9NotificationWidget* GetNotificationWidget() const;

private:
	void EnsureNotificationWidget();
	TSubclassOf<ULoop9NotificationWidget> ResolveNotificationWidgetClass() const;

	UPROPERTY(Transient)
	TObjectPtr<ULoop9NotificationWidget> NotificationWidget;
};
