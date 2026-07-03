#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Loop9Interactable.h"
#include "AI_Friend.generated.h"

UCLASS(Config = Game)
class LOOP9_API AAI_Friend : public AActor, public ILoop9Interactable
{
	GENERATED_BODY()

public:
	AAI_Friend();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PhoneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual bool TryInteract_Implementation(APlayerController* InteractingController) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Limits", meta = (ClampMin = "1"))
	int32 MaxMessagesPerLoop = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Limits")
	bool bUseSignalDropOnLimit = true;

	UFUNCTION(BlueprintCallable, Category = "AI")
	FString SayToAI(const FString& Message);

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString APIEndpoint;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString GameToken;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString PlayerId;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString PreferredLanguage = TEXT("sr");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> ChatWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Scripted")
    FString InitialRuleMessage = TEXT("Listen carefully: if you notice any irregularity, take the elevator that has interior light on (RESTART). If you find no anomaly, take the elevator without interior light (NEXT). The first loop is clean, so take your time and learn the baseline.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	bool bAutoRingOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring", meta = (ClampMin = "1"))
	int32 InitialRingRepeatCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	TObjectPtr<class USoundBase> InitialRingSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	TObjectPtr<class USoundAttenuation> InitialRingAttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	TObjectPtr<class USoundBase> PhoneInteractSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring")
	TObjectPtr<class USoundAttenuation> PhoneInteractAttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Phone Ring", meta = (ClampMin = "0.05"))
	float InitialRingInterval = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OpenChatWidget(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CloseChatWidget(APlayerController* PlayerController);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPlayerEnterRange();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPlayerExitRange();

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bPlayerInRange;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	APlayerController* CurrentPlayerController;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	UUserWidget* GetChatWidgetInstance() const { return ChatWidgetInstance; }

private:
	int32 ResolveCurrentLoopIndex() const;
	void RefreshLoopMessageLimitCounter();
	void LoadConfiguredOverrides();
	void StopInitialRing();
	void StartInitialRingIfNeeded();
	bool ShouldStopInitialRingForLoopChange() const;
	class UAI_ChatWidget* GetChatWidgetTyped() const;
	class UAI_ChatWidget* GetOrCreateChatWidgetTyped(APlayerController* PlayerController);
	void SetPlayerMovementEnabled(APlayerController* PlayerController, bool bEnabled) const;
	void ApplyInteractionInputMode(APlayerController* PlayerController, bool bUIOnly) const;

	void OnResponseReceived(FString Response);
	FString LastAIResponse;
	FString BuildSignalDropMessage() const;

	void TriggerInitialRing();

	UPROPERTY()
	UUserWidget* ChatWidgetInstance;

	FTimerHandle InitialRingTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> ActiveInitialRingAudioComponent = nullptr;

	int32 RingsPlayedCount = 0;
	bool bInitialMessageInjected = false;
	int32 InitialLoopNumberAtBeginPlay = 1;
	int32 LastLoopIndexForMessageLimit = INDEX_NONE;
	int32 MessagesSentThisLoop = 0;

	UFUNCTION()
   void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
 void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
