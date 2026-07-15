// Fill out your copyright notice in the Description page of Project Settings.

#include "Loop9SettingRowWidgets.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"

// --- ULoop9SettingRowBase ---

void ULoop9SettingRowBase::NativePreConstruct()
{
	Super::NativePreConstruct();
	ResolveBoundWidgets();

	if (Text_Label && !Label.IsEmpty())
	{
		Text_Label->SetText(Label);
	}
}

void ULoop9SettingRowBase::SetLabel(const FText& InLabel)
{
	Label = InLabel;
	ResolveBoundWidgets();
	if (Text_Label)
	{
		Text_Label->SetText(Label);
	}
}

void ULoop9SettingRowBase::ResolveBoundWidgets()
{
	if (!Text_Label)
	{
		Text_Label = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Label")));
	}
}

// --- ULoop9SliderRow ---

void ULoop9SliderRow::ResolveBoundWidgets()
{
	ULoop9SettingRowBase::ResolveBoundWidgets();

	if (!Slider_Value)
	{
		Slider_Value = Cast<USlider>(GetWidgetFromName(TEXT("Slider_Value")));
	}
	if (!Text_Value)
	{
		Text_Value = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Value")));
	}
}

void ULoop9SliderRow::NativePreConstruct()
{
	Super::NativePreConstruct();
	ResolveBoundWidgets();

	if (Slider_Value)
	{
		UpdateValueText(Slider_Value->GetValue());
	}
}

void ULoop9SliderRow::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveBoundWidgets();

	if (Slider_Value)
	{
		Slider_Value->OnValueChanged.RemoveDynamic(this, &ULoop9SliderRow::HandleSliderChanged);
		Slider_Value->OnValueChanged.AddDynamic(this, &ULoop9SliderRow::HandleSliderChanged);
	}
}

void ULoop9SliderRow::HandleSliderChanged(float InValue)
{
	UpdateValueText(InValue);
	OnValueChanged.Broadcast(InValue);
}

void ULoop9SliderRow::SetValue(float InValue)
{
	ResolveBoundWidgets();
	if (Slider_Value)
	{
		Slider_Value->SetValue(InValue);
	}
	UpdateValueText(InValue);
}

float ULoop9SliderRow::GetValue() const
{
	if (Slider_Value)
	{
		return Slider_Value->GetValue();
	}

	if (UWidget* Found = const_cast<ULoop9SliderRow*>(this)->GetWidgetFromName(TEXT("Slider_Value")))
	{
		if (USlider* AsSlider = Cast<USlider>(Found))
		{
			return AsSlider->GetValue();
		}
	}

	return 0.0f;
}

void ULoop9SliderRow::UpdateValueText(float InValue)
{
	if (!Text_Value)
	{
		return;
	}

	if (bShowValueAsPercent)
	{
		Text_Value->SetText(FText::Format(NSLOCTEXT("Loop9Settings", "RowValuePercent", "{0}%"),
			FText::AsNumber(FMath::RoundToInt(InValue * 100.0f))));
	}
	else
	{
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits = 2;
		Text_Value->SetText(FText::AsNumber(InValue, &Options));
	}
}

// --- ULoop9ComboRow ---

void ULoop9ComboRow::ResolveBoundWidgets()
{
	ULoop9SettingRowBase::ResolveBoundWidgets();
	if (!ComboBox_Value)
	{
		ComboBox_Value = Cast<UComboBoxString>(GetWidgetFromName(TEXT("ComboBox_Value")));
	}
}

void ULoop9ComboRow::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveBoundWidgets();

	if (ComboBox_Value)
	{
		ComboBox_Value->OnSelectionChanged.RemoveDynamic(this, &ULoop9ComboRow::HandleSelectionChanged);
		ComboBox_Value->OnSelectionChanged.AddDynamic(this, &ULoop9ComboRow::HandleSelectionChanged);
	}
}

void ULoop9ComboRow::HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	OnSelectionChanged.Broadcast(SelectedItem, SelectionType);
}

void ULoop9ComboRow::ClearOptions()
{
	ResolveBoundWidgets();
	if (ComboBox_Value)
	{
		ComboBox_Value->ClearOptions();
	}
}

void ULoop9ComboRow::AddOption(const FString& Option)
{
	ResolveBoundWidgets();
	if (ComboBox_Value)
	{
		ComboBox_Value->AddOption(Option);
	}
}

int32 ULoop9ComboRow::FindOptionIndex(const FString& Option) const
{
	return ComboBox_Value ? ComboBox_Value->FindOptionIndex(Option) : INDEX_NONE;
}

void ULoop9ComboRow::SetSelectedIndex(int32 Index)
{
	ResolveBoundWidgets();
	if (ComboBox_Value)
	{
		ComboBox_Value->SetSelectedIndex(Index);
	}
}

void ULoop9ComboRow::SetSelectedOption(const FString& Option)
{
	ResolveBoundWidgets();
	if (ComboBox_Value)
	{
		ComboBox_Value->SetSelectedOption(Option);
	}
}

int32 ULoop9ComboRow::GetSelectedIndex() const
{
	return ComboBox_Value ? ComboBox_Value->GetSelectedIndex() : INDEX_NONE;
}

FString ULoop9ComboRow::GetSelectedOption() const
{
	return ComboBox_Value ? ComboBox_Value->GetSelectedOption() : FString();
}

void ULoop9ComboRow::RefreshSelectionDisplay()
{
	ResolveBoundWidgets();
	if (!ComboBox_Value)
	{
		return;
	}

	const int32 Index = ComboBox_Value->GetSelectedIndex();
	if (Index == INDEX_NONE)
	{
		return;
	}

	const FString Option = ComboBox_Value->GetOptionAtIndex(Index);
	// Clear then re-select so the closed combo content widget rebuilds with the new string.
	ComboBox_Value->ClearSelection();
	ComboBox_Value->SetSelectedOption(Option);
}

// --- ULoop9CheckRow ---

void ULoop9CheckRow::ResolveBoundWidgets()
{
	ULoop9SettingRowBase::ResolveBoundWidgets();
	if (!CheckBox_Value)
	{
		CheckBox_Value = Cast<UCheckBox>(GetWidgetFromName(TEXT("CheckBox_Value")));
	}
}

void ULoop9CheckRow::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveBoundWidgets();

	if (CheckBox_Value)
	{
		CheckBox_Value->OnCheckStateChanged.RemoveDynamic(this, &ULoop9CheckRow::HandleCheckStateChanged);
		CheckBox_Value->OnCheckStateChanged.AddDynamic(this, &ULoop9CheckRow::HandleCheckStateChanged);
	}
}

void ULoop9CheckRow::HandleCheckStateChanged(bool bIsChecked)
{
	OnCheckStateChanged.Broadcast(bIsChecked);
}

void ULoop9CheckRow::SetIsChecked(bool bIsChecked)
{
	ResolveBoundWidgets();
	if (CheckBox_Value)
	{
		CheckBox_Value->SetIsChecked(bIsChecked);
	}
}

bool ULoop9CheckRow::IsChecked() const
{
	if (CheckBox_Value)
	{
		return CheckBox_Value->IsChecked();
	}

	if (UWidget* Found = const_cast<ULoop9CheckRow*>(this)->GetWidgetFromName(TEXT("CheckBox_Value")))
	{
		if (UCheckBox* AsCheck = Cast<UCheckBox>(Found))
		{
			return AsCheck->IsChecked();
		}
	}

	return false;
}
