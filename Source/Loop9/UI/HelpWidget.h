#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HelpWidget.generated.h"

class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHelpClosed);

/**
 * Main-menu briefing that states the rules the game otherwise only implies.
 *
 * The opening phone call gives the player the rule once, in the dark, while
 * they are still learning to walk. Anyone who misses it spends several loops
 * guessing which elevator means what, and a wrong guess is indistinguishable
 * from a bug. This screen is the place they can go back and check.
 *
 * Like the Shift Archive, it builds its own layout in C++ when the Blueprint
 * child has no designer root, so the screen is never blank.
 */
UCLASS()
class LOOP9_API UHelpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Help")
	void SetReturnTarget(UUserWidget* Target);

	UFUNCTION(BlueprintCallable, Category = "Help")
	void RequestClose();

	UPROPERTY(BlueprintAssignable, Category = "Help")
	FOnHelpClosed OnClosed;

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
