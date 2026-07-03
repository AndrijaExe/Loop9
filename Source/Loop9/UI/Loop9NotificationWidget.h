#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Loop9NotificationWidget.generated.h"

class UTextBlock;

USTRUCT(BlueprintType)
struct FLoop9NotificationEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification", meta = (ClampMin = "0.0"))
	float DisplayDuration = 4.0f;
};

UCLASS()
class LOOP9_API ULoop9NotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications")
	void AddMessage(const FText& Message, float TimeDuration = 4.0f);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications")
	void ShowNotification(const FText& Message, float DisplayDuration = 4.0f);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Notifications")
	void ClearNotificationQueue();

	UFUNCTION(BlueprintPure, Category = "Notification")
	bool IsShowingNotification() const { return bIsShowingNotification; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification", meta = (ClampMin = "0.0"))
	float DefaultDisplayDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification", meta = (ClampMin = "0.0"))
	float MessageGapDuration = 0.35f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Notification", meta = (DisplayName = "On Notification Shown"))
	void BP_OnNotificationShown(const FText& Message, float DisplayDuration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Notification", meta = (DisplayName = "On Notification Hidden"))
	void BP_OnNotificationHidden();

private:
	void BuildFallbackLayoutIfNeeded();
	void DisplayNextNotification();
	void FinishCurrentNotification();
	void RefreshMessageText(const FText& Message);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FallbackMessageText;

	TArray<FLoop9NotificationEntry> PendingNotifications;
	FTimerHandle HideTimerHandle;
	FTimerHandle GapTimerHandle;
	bool bIsShowingNotification = false;
};
