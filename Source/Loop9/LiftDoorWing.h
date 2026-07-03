// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LiftDoorWing.generated.h"

UCLASS()
class LOOP9_API ALiftDoorWing : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALiftDoorWing();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    // Whether the wing is currently moving (read-only in editor)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
    bool ShouldMove;

    // Whether the wing is currently opened (read-only in editor)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
    bool IsOpened;

    // Movement speed in units per second (editable in editor / blueprints)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.0"))
    float MovementSpeed;

    // How many units (cm) the wing should move when opening (editable)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.0"))
    float TravelDistance;

    // Direction in which the wing moves when opening (editable)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    FVector MovementDirection;

    // Opens the door wing (starts movement towards open position)
    UFUNCTION(BlueprintCallable, Category = "Door")
    void OpenDoorWing();

    // Closes the door wing (starts movement towards closed position)
    UFUNCTION(BlueprintCallable, Category = "Door")
    void CloseDoorWing();

private:
    // Starting location for the current movement
    FVector StartLocation;

    // Target location for the current movement
    FVector TargetLocation;

    // Whether the current movement is opening (true) or closing (false)
    bool bMovingToOpen;
    
    // The closed (initial) location of the wing
    FVector ClosedLocation;

};
