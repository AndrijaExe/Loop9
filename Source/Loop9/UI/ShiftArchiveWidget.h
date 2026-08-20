#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Loop/LoopTypes.h"
#include "ShiftArchiveWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShiftArchiveClosed);

/** Main-menu dossier: SMENA hub plus six ending nodes from SeenEndings. */
UCLASS()
class LOOP9_API UShiftArchiveWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Archive")
	void SetReturnTarget(UUserWidget* Target);

	UFUNCTION(BlueprintCallable, Category = "Archive")
	void RequestClose();

	UPROPERTY(BlueprintAssignable, Category = "Archive")
	FOnShiftArchiveClosed OnClosed;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_Nodes;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Title;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BT_Back;

	UFUNCTION()
	void HandleBackClicked();

private:
	void BuildFallbackLayoutIfNeeded();
	void BindBackButton();
	void RefreshNodes();
	void AddNodeRow(UVerticalBox* Parent, const FText& Label, bool bUnlocked, bool bIsHub);
	static FText TitleForEnding(ELoopEndingType EndingType);

	UPROPERTY()
	TObjectPtr<UUserWidget> ReturnTarget;

	UPROPERTY()
	TObjectPtr<UButton> FallbackBackButton;
};
