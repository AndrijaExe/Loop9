#include "Subsystems/Loop9GameplayNotificationSubsystem.h"

#include "Controllers/Loop9PlayerController.h"
#include "Loop9GameMode.h"
#include "UI/Loop9NotificationWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ULoop9GameplayNotificationSubsystem* ULoop9GameplayNotificationSubsystem::GetGameplayNotifications(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULoop9GameplayNotificationSubsystem>() : nullptr;
}

void ULoop9GameplayNotificationSubsystem::AddMessage(const FText& Message, float TimeDuration)
{
	if (Message.IsEmpty())
	{
		return;
	}

	EnsureNotificationWidget();

	if (NotificationWidget)
	{
		NotificationWidget->AddMessage(Message, TimeDuration);
	}
}

void ULoop9GameplayNotificationSubsystem::AddMessageString(const FString& Message, float TimeDuration)
{
	AddMessage(FText::FromString(Message), TimeDuration);
}

void ULoop9GameplayNotificationSubsystem::ClearMessages()
{
	if (NotificationWidget)
	{
		NotificationWidget->ClearNotificationQueue();
	}
}

ULoop9NotificationWidget* ULoop9GameplayNotificationSubsystem::GetNotificationWidget() const
{
	return NotificationWidget;
}

void ULoop9GameplayNotificationSubsystem::EnsureNotificationWidget()
{
	if (NotificationWidget)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		return;
	}

	const TSubclassOf<ULoop9NotificationWidget> WidgetClass = ResolveNotificationWidgetClass();
	NotificationWidget = CreateWidget<ULoop9NotificationWidget>(PlayerController, WidgetClass);
	if (NotificationWidget)
	{
		NotificationWidget->AddToViewport(100);
	}
}

TSubclassOf<ULoop9NotificationWidget> ULoop9GameplayNotificationSubsystem::ResolveNotificationWidgetClass() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return ULoop9NotificationWidget::StaticClass();
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (const ALoop9PlayerController* LoopPlayerController = Cast<ALoop9PlayerController>(PlayerController))
		{
			if (LoopPlayerController->NotificationWidgetClass)
			{
				return LoopPlayerController->NotificationWidgetClass;
			}
		}
	}

	if (const ALoop9GameMode* LoopGameMode = Cast<ALoop9GameMode>(UGameplayStatics::GetGameMode(World)))
	{
		if (LoopGameMode->GameplayNotificationWidgetClass)
		{
			return LoopGameMode->GameplayNotificationWidgetClass;
		}
	}

	return ULoop9NotificationWidget::StaticClass();
}
