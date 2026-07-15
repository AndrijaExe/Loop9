// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "Loop9SettingRowWidgets.generated.h"

class UTextBlock;
class USlider;
class UComboBoxString;
class UCheckBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoop9RowFloatChanged, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoop9RowBoolChanged, bool, bIsChecked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLoop9RowSelectionChanged, FString, SelectedItem, ESelectInfo::Type, SelectionType);

/**
 * Base for a standardized settings row: a label next to an input control.
 * Blueprint children (WBP_SliderRow, WBP_ComboRow, WBP_CheckRow) provide the
 * visuals once; every row in the settings screen reuses them.
 */
UCLASS(Abstract)
class LOOP9_API ULoop9SettingRowBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Text shown in the row label. Editable per-instance in the designer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting Row")
	FText Label;

	UFUNCTION(BlueprintCallable, Category = "Setting Row")
	void SetLabel(const FText& InLabel);

protected:
	virtual void NativePreConstruct() override;

	void ResolveBoundWidgets();

	UPROPERTY(BlueprintReadOnly, Category = "Setting Row", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> Text_Label;
};

/** Label + slider (+ optional live value readout). */
UCLASS()
class LOOP9_API ULoop9SliderRow : public ULoop9SettingRowBase
{
	GENERATED_BODY()

public:
	/** Fired when the user moves the slider. */
	UPROPERTY(BlueprintAssignable, Category = "Setting Row")
	FLoop9RowFloatChanged OnValueChanged;

	/** How the optional value text is formatted: value * 100 shown as "85%". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting Row")
	bool bShowValueAsPercent = true;

	UFUNCTION(BlueprintCallable, Category = "Setting Row")
	void SetValue(float InValue);

	UFUNCTION(BlueprintPure, Category = "Setting Row")
	float GetValue() const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleSliderChanged(float InValue);

	void UpdateValueText(float InValue);
	void ResolveBoundWidgets();

	UPROPERTY(BlueprintReadOnly, Category = "Setting Row", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<USlider> Slider_Value;

	UPROPERTY(BlueprintReadOnly, Category = "Setting Row", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> Text_Value;
};

/** Label + combo box. Mirrors the UComboBoxString API used by USettingsWidget. */
UCLASS()
class LOOP9_API ULoop9ComboRow : public ULoop9SettingRowBase
{
	GENERATED_BODY()

public:
	/** Fired when the user picks an option. */
	UPROPERTY(BlueprintAssignable, Category = "Setting Row")
	FLoop9RowSelectionChanged OnSelectionChanged;

	UFUNCTION(BlueprintCallable, Category = "Setting Row")
	void ClearOptions();

	UFUNCTION(BlueprintCallable, Category = "Setting Row")
	void AddOption(const FString& Option);

	UFUNCTION(BlueprintPure, Category = "Setting Row")
	int32 FindOptionIndex(const FString& Option) const;

	UFUNCTION(BlueprintCallable, Category = "Setting Row")
	void SetSelectedIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Setting Row")
	void SetSelectedOption(const FString& Option);

	UFUNCTION(BlueprintPure, Category = "Setting Row")
	int32 GetSelectedIndex() const;

	UFUNCTION(BlueprintPure, Category = "Setting Row")
	FString GetSelectedOption() const;

	/** Re-applies the current selection so the closed-combo label redraws after options change. */
	UFUNCTION(BlueprintCallable, Category = "Setting Row")
	void RefreshSelectionDisplay();

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	void ResolveBoundWidgets();

	UPROPERTY(BlueprintReadOnly, Category = "Setting Row", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UComboBoxString> ComboBox_Value;
};

/** Label + checkbox. */
UCLASS()
class LOOP9_API ULoop9CheckRow : public ULoop9SettingRowBase
{
	GENERATED_BODY()

public:
	/** Fired when the user toggles the checkbox. */
	UPROPERTY(BlueprintAssignable, Category = "Setting Row")
	FLoop9RowBoolChanged OnCheckStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Setting Row")
	void SetIsChecked(bool bIsChecked);

	UFUNCTION(BlueprintPure, Category = "Setting Row")
	bool IsChecked() const;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleCheckStateChanged(bool bIsChecked);

	void ResolveBoundWidgets();

	UPROPERTY(BlueprintReadOnly, Category = "Setting Row", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UCheckBox> CheckBox_Value;
};
