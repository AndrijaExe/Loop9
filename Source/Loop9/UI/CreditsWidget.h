#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreditsWidget.generated.h"

class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreditsClosed);

/**
 * Main-menu credits. Attribution lives here, not in Help: Help is how to play,
 * this is who made the sounds. Builds its own layout when the Blueprint child
 * has no designer root, same pattern as Help and the Shift Archive.
 */
UCLASS()
class LOOP9_API UCreditsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Credits")
	void SetReturnTarget(UUserWidget* Target);

	UFUNCTION(BlueprintCallable, Category = "Credits")
	void RequestClose();

	UPROPERTY(BlueprintAssignable, Category = "Credits")
	FOnCreditsClosed OnClosed;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SB_Sections;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_Sections;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Title;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BT_Back;

	UFUNCTION()
	void HandleBackClicked();

private:
	void BuildFallbackLayoutIfNeeded();
	void BindBackButton();
	void RefreshSections();
	void AddSection(UVerticalBox* Parent, const FText& Heading, const FText& Body);

	UPROPERTY()
	TObjectPtr<UUserWidget> ReturnTarget;

	UPROPERTY()
	TObjectPtr<UButton> FallbackBackButton;
};
