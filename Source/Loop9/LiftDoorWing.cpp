// Fill out your copyright notice in the Description page of Project Settings.


#include "LiftDoorWing.h"

// Sets default values
ALiftDoorWing::ALiftDoorWing()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // sensible defaults
    ShouldMove = false;
    IsOpened = false;
    MovementSpeed = 100.f;
    TravelDistance = 200.f;
    MovementDirection = FVector(1.f, 0.f, 0.f);
    bMovingToOpen = false;
    StartLocation = FVector::ZeroVector;
    TargetLocation = FVector::ZeroVector;
    ClosedLocation = FVector::ZeroVector;
}

// Called when the game starts or when spawned
void ALiftDoorWing::BeginPlay()
{
	Super::BeginPlay();

    // record closed location on begin play
    ClosedLocation = GetActorLocation();
    StartLocation = ClosedLocation;
    TargetLocation = ClosedLocation;

    // Start opening on begin play. If you prefer BP control, remove this call.
    OpenDoorWing();
}

// Called every frame
void ALiftDoorWing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (!ShouldMove)
    {
        return;
    }

    FVector Current = GetActorLocation();
    FVector ToTarget = TargetLocation - Current;
    float DistToTarget = ToTarget.Size();

    if (DistToTarget <= KINDA_SMALL_NUMBER)
    {
        // reached
        SetActorLocation(TargetLocation);
        ShouldMove = false;
        IsOpened = bMovingToOpen;
        return;
    }

    FVector Dir = ToTarget.GetSafeNormal();
    float MoveStep = MovementSpeed * DeltaTime;

    if (MoveStep >= DistToTarget)
    {
        SetActorLocation(TargetLocation);
        ShouldMove = false;
        IsOpened = bMovingToOpen;
    }
    else
    {
        SetActorLocation(Current + Dir * MoveStep);
    }
}

void ALiftDoorWing::OpenDoorWing()
{
    if (ShouldMove && bMovingToOpen)
    {
        return; // already moving to open
    }

    StartLocation = GetActorLocation();
    TargetLocation = ClosedLocation + MovementDirection.GetSafeNormal() * TravelDistance;
    bMovingToOpen = true;
    ShouldMove = true;

    UE_LOG(LogTemp, Log, TEXT("OpenDoorWing called. Speed=%.1f Travel=%.1f Dir=(%.2f,%.2f,%.2f) Start=(%.1f,%.1f,%.1f) Target=(%.1f,%.1f,%.1f)"),
        MovementSpeed, TravelDistance,
        MovementDirection.X, MovementDirection.Y, MovementDirection.Z,
        StartLocation.X, StartLocation.Y, StartLocation.Z,
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
}

void ALiftDoorWing::CloseDoorWing()
{
    if (ShouldMove && !bMovingToOpen)
    {
        return; // already moving to close
    }

    StartLocation = GetActorLocation();
    TargetLocation = ClosedLocation;
    bMovingToOpen = false;
    ShouldMove = true;

    UE_LOG(LogTemp, Log, TEXT("CloseDoorWing called. Start=(%.1f,%.1f,%.1f) Target=(%.1f,%.1f,%.1f)"),
        StartLocation.X, StartLocation.Y, StartLocation.Z,
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
}

