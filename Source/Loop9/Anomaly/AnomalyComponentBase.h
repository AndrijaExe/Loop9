#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Anomaly/AnomalyTypes.h"
#include "AnomalyComponentBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnomalyStateChanged, bool, bIsActive);

UCLASS(Abstract, BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOOP9_API UAnomalyComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UAnomalyComponentBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnomalyProbability = 0.5f;

	/**
	 * Relative odds of being drawn out of this component's own type pool, where
	 * 1.0 is the norm and 2.0 is picked twice as often as a sibling at 1.0.
	 *
	 * This is not the same knob as AnomalyProbability: the manager first draws a
	 * type, then a component inside that type, and only then rolls
	 * AnomalyProbability to decide whether the draw activates. Weight moves a
	 * component up the queue; probability decides whether its turn counts.
	 * Raising the weight is the safe way to favour one placement over another,
	 * because it never changes how full the floor ends up being.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "0.01"))
	float SelectionWeight = 1.0f;

	/**
	 * Coarse landmark the player would recognise from the screen, written in
	 * English, e.g. "the north corridor". The AI is told the place but not the
	 * item, so it can point without solving the search. Leave empty for
	 * anomalies that have no place, such as a phantom chat message.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly|AI Context")
	FString AnomalyZone;

	/**
	 * English category noun, e.g. "a ceiling light panel". Never an actor name:
	 * this can reach the model, and "SM_Lamp_03" breaks the fiction.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly|AI Context")
	FString AnomalyObjectKind;

	UPROPERTY(BlueprintReadOnly, Category = "Anomaly")
	bool bIsAnomalyActive = false;

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	virtual void ActivateAnomaly();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	virtual void DeactivateAnomaly();

	/** Ensures world state is normal when the anomaly is inactive (override for hide-style cleanup). */
	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	virtual void ResetToNormalState();

	virtual ELoopAnomalyType GetAnomalyType() const PURE_VIRTUAL(UAnomalyComponentBase::GetAnomalyType, return ELoopAnomalyType::Hide;);

	FName GetAnomalyTypeLabel() const { return GetLoopAnomalyTypeLabel(GetAnomalyType()); }

	UPROPERTY(BlueprintAssignable, Category = "Anomaly")
	FOnAnomalyStateChanged OnAnomalyActivated;

	UPROPERTY(BlueprintAssignable, Category = "Anomaly")
	FOnAnomalyStateChanged OnAnomalyDeactivated;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Returns true when anomaly state was applied successfully. */
	virtual bool ApplyAnomalyState();
	virtual void RestoreNormalState();
};
