// Copyright Epic Games, Inc. All Rights Reserved.


#include "HorrorUI.h"
#include "Loop9Character.h"
#include "Components/TextBlock.h"

void UHorrorUI::NativeDestruct()
{
	if (BoundCharacter.IsValid())
	{
		BoundCharacter->OnSprintMeterUpdated.RemoveDynamic(this, &UHorrorUI::OnSprintMeterUpdated);
		BoundCharacter->OnSprintStateChanged.RemoveDynamic(this, &UHorrorUI::OnSprintStateChanged);
		BoundCharacter.Reset();
	}

	Super::NativeDestruct();
}

void UHorrorUI::SetupCharacter(ALoop9Character* Character)
{
	if (!Character)
	{
		return;
	}

	if (BoundCharacter.IsValid())
	{
		if (BoundCharacter.Get() == Character)
		{
			return;
		}

		BoundCharacter->OnSprintMeterUpdated.RemoveDynamic(this, &UHorrorUI::OnSprintMeterUpdated);
		BoundCharacter->OnSprintStateChanged.RemoveDynamic(this, &UHorrorUI::OnSprintStateChanged);
	}

	BoundCharacter = Character;
	Character->OnSprintMeterUpdated.AddDynamic(this, &UHorrorUI::OnSprintMeterUpdated);
	Character->OnSprintStateChanged.AddDynamic(this, &UHorrorUI::OnSprintStateChanged);
	OnSprintMeterUpdated(Character->GetSprintMeterPercent());
	OnSprintStateChanged(Character->IsSprintActive());
}

void UHorrorUI::OnSprintMeterUpdated(float Percent)
{
	BP_SprintMeterUpdated(Percent);
}

void UHorrorUI::OnSprintStateChanged(bool bSprinting)
{
	BP_SprintStateChanged(bSprinting);
}

void UHorrorUI::SetInteractionPrompt_Implementation(const FText& PromptText, bool bVisible)
{
	SetInteractionPrompt(PromptText, bVisible);
}

void UHorrorUI::SetInteractionPrompt(const FText& PromptText, bool bVisible)
{
	if (TB_InteractionPrompt)
	{
		TB_InteractionPrompt->SetText(PromptText);
		TB_InteractionPrompt->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	BP_InteractionPromptUpdated(PromptText, bVisible);
}

void UHorrorUI::SetCrosshairVisible(bool bVisible)
{
	if (TB_CrosshairDot)
	{
		TB_CrosshairDot->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	BP_CrosshairVisibilityChanged(bVisible);
}
