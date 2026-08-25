#include "UI/Loop9GameplayNotificationLibrary.h"

#include "Subsystems/Loop9GameplayNotificationSubsystem.h"

void ULoop9GameplayNotificationLibrary::AddGameplayMessage(UObject* WorldContextObject, const FText& Message, float TimeDuration)
{
	if (ULoop9GameplayNotificationSubsystem* Notifications = ULoop9GameplayNotificationSubsystem::GetGameplayNotifications(WorldContextObject))
	{
		Notifications->AddMessage(Message, TimeDuration);
	}
}

void ULoop9GameplayNotificationLibrary::AddGameplayMessageString(UObject* WorldContextObject, const FString& Message, float TimeDuration)
{
	if (ULoop9GameplayNotificationSubsystem* Notifications = ULoop9GameplayNotificationSubsystem::GetGameplayNotifications(WorldContextObject))
	{
		Notifications->AddMessageString(Message, TimeDuration);
	}
}
