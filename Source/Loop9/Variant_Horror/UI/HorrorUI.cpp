// Copyright Epic Games, Inc. All Rights Reserved.


#include "HorrorUI.h"
#include "HorrorCharacter.h"
#include "Components/TextBlock.h"

void UHorrorUI::SetupCharacter(AHorrorCharacter* HorrorCharacter)
{
   if (!HorrorCharacter)
	{
		return;
	}

	HorrorCharacter->OnSprintMeterUpdated.AddDynamic(this, &UHorrorUI::OnSprintMeterUpdated);
	HorrorCharacter->OnSprintStateChanged.AddDynamic(this, &UHorrorUI::OnSprintStateChanged);
}

void UHorrorUI::OnSprintMeterUpdated(float Percent)
{
	// call the BP handler
	BP_SprintMeterUpdated(Percent);
}

void UHorrorUI::OnSprintStateChanged(bool bSprinting)
{
	// call the BP handler
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
