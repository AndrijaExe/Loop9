#include "UI/Loop9WidgetClickBinder.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UObject/UnrealType.h"

namespace Loop9ClickBinderPrivate
{
	static UButton* FindNestedButton(UWidget* Widget)
	{
		if (UButton* AsButton = Cast<UButton>(Widget))
		{
			return AsButton;
		}

		UUserWidget* AsUserWidget = Cast<UUserWidget>(Widget);
		if (!AsUserWidget)
		{
			return nullptr;
		}

		static const FName ButtonNames[] = {
			TEXT("Button"),
			TEXT("Button_0"),
			TEXT("MainButton")
		};

		for (const FName& Name : ButtonNames)
		{
			if (UButton* Found = Cast<UButton>(AsUserWidget->GetWidgetFromName(Name)))
			{
				return Found;
			}
		}

		return nullptr;
	}
}

bool FLoop9WidgetClickBinder::BindClicked(UWidget* Widget, UObject* Target, FName FunctionName)
{
	if (!Widget || !Target || FunctionName.IsNone())
	{
		return false;
	}

	if (UButton* AsButton = Cast<UButton>(Widget))
	{
		FScriptDelegate Del;
		Del.BindUFunction(Target, FunctionName);
		AsButton->OnClicked.AddUnique(Del);
		return true;
	}

	if (FProperty* Prop = Widget->GetClass()->FindPropertyByName(TEXT("OnButtonClicked")))
	{
		if (FMulticastDelegateProperty* MDP = CastField<FMulticastDelegateProperty>(Prop))
		{
			FScriptDelegate Del;
			Del.BindUFunction(Target, FunctionName);
			MDP->AddDelegate(Del, Widget);
			return true;
		}
	}

	// Fallback: bind the nested UButton inside WBP_Button.
	if (UButton* Nested = Loop9ClickBinderPrivate::FindNestedButton(Widget))
	{
		FScriptDelegate Del;
		Del.BindUFunction(Target, FunctionName);
		Nested->OnClicked.AddUnique(Del);
		return true;
	}

	return false;
}

void FLoop9WidgetClickBinder::UnbindClicked(UWidget* Widget, UObject* Target, FName FunctionName)
{
	if (!Widget || !Target || FunctionName.IsNone())
	{
		return;
	}

	if (UButton* AsButton = Cast<UButton>(Widget))
	{
		FScriptDelegate Del;
		Del.BindUFunction(Target, FunctionName);
		AsButton->OnClicked.Remove(Del);
		return;
	}

	if (FProperty* Prop = Widget->GetClass()->FindPropertyByName(TEXT("OnButtonClicked")))
	{
		if (FMulticastDelegateProperty* MDP = CastField<FMulticastDelegateProperty>(Prop))
		{
			FScriptDelegate Del;
			Del.BindUFunction(Target, FunctionName);
			MDP->RemoveDelegate(Del, Widget);
		}
	}

	if (UButton* Nested = Loop9ClickBinderPrivate::FindNestedButton(Widget))
	{
		FScriptDelegate Del;
		Del.BindUFunction(Target, FunctionName);
		Nested->OnClicked.Remove(Del);
	}
}

void FLoop9WidgetClickBinder::SetButtonText(UWidget* Widget, const FText& InText)
{
	if (!Widget)
	{
		return;
	}

	if (FProperty* Prop = Widget->GetClass()->FindPropertyByName(TEXT("ButtonText")))
	{
		if (FTextProperty* TextProp = CastField<FTextProperty>(Prop))
		{
			TextProp->SetPropertyValue_InContainer(Widget, InText);
		}
	}

	// WBP_Button stores display text in an inner TextBlock; update it live.
	if (UUserWidget* AsUserWidget = Cast<UUserWidget>(Widget))
	{
		static const FName TextBlockNames[] = {
			TEXT("TextBlock_129"),
			TEXT("Text_Label"),
			TEXT("ButtonLabel"),
			TEXT("Text")
		};

		for (const FName& Name : TextBlockNames)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(AsUserWidget->GetWidgetFromName(Name)))
			{
				TextBlock->SetText(InText);
				return;
			}
		}
	}
}
