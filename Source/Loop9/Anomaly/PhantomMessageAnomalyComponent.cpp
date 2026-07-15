#include "Anomaly/PhantomMessageAnomalyComponent.h"

#include "AI_Friend.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "Loop9Anomaly"

UPhantomMessageAnomalyComponent::UPhantomMessageAnomalyComponent()
{
	AnomalyProbability = 0.5f;

	PhantomMessages = {
		LOCTEXT("PhantomMsg1", "I already told you about the monitor. Why do you keep asking?"),
		LOCTEXT("PhantomMsg2", "help"),
		LOCTEXT("PhantomMsg3", "I'm not going back down there."),
		LOCTEXT("PhantomMsg4", "Did you hear that too?"),
	};
}

AAI_Friend* UPhantomMessageAnomalyComponent::FindAIFriend() const
{
	if (AAI_Friend* OwnerFriend = Cast<AAI_Friend>(GetOwner()))
	{
		return OwnerFriend;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AAI_Friend> It(World); It; ++It)
		{
			return *It;
		}
	}

	return nullptr;
}

bool UPhantomMessageAnomalyComponent::ApplyAnomalyState()
{
	if (PhantomMessages.Num() == 0)
	{
		return false;
	}

	AAI_Friend* AIFriend = FindAIFriend();
	if (!AIFriend)
	{
		return false;
	}

	const FText& Message = PhantomMessages[FMath::RandRange(0, PhantomMessages.Num() - 1)];
	AIFriend->QueuePhantomPlayerMessage(Message.ToString());
	return true;
}

void UPhantomMessageAnomalyComponent::RestoreNormalState()
{
	if (AAI_Friend* AIFriend = FindAIFriend())
	{
		AIFriend->ClearPhantomPlayerMessage();
	}
}

#undef LOCTEXT_NAMESPACE
