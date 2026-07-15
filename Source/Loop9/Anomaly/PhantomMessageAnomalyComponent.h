#pragma once

#include "CoreMinimal.h"
#include "Anomaly/AnomalyComponentBase.h"
#include "PhantomMessageAnomalyComponent.generated.h"

/**
 * Psychological anomaly: when the player opens the chat, there is a message
 * in the history that they never sent - shown with the player's own prefix.
 *
 * Place on the AI_Friend (phone) actor, or anywhere in the level; the
 * component finds the AI_Friend automatically. The message only becomes
 * visible when the chat is opened, so spotting it requires answering the
 * phone - which is exactly the paranoia this anomaly is meant to create.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UPhantomMessageAnomalyComponent : public UAnomalyComponentBase
{
	GENERATED_BODY()

public:
	UPhantomMessageAnomalyComponent();

	virtual ELoopAnomalyType GetAnomalyType() const override { return ELoopAnomalyType::PhantomMessage; }

	/** Pool of fake "player" messages; one is picked at random per activation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom Message Anomaly")
	TArray<FText> PhantomMessages;

protected:
	virtual bool ApplyAnomalyState() override;
	virtual void RestoreNormalState() override;

private:
	class AAI_Friend* FindAIFriend() const;
};
