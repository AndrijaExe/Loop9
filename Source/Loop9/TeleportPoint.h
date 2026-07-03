// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "TeleportPoint.generated.h"

UENUM(BlueprintType)
enum class ECustomTeleportType : uint8
{
	Entry UMETA(DisplayName = "Entry Point"),
	Exit UMETA(DisplayName = "Exit Point")
};

UCLASS()
class LOOP9_API ATeleportPoint : public AActor
{
	GENERATED_BODY()

public:
	ATeleportPoint();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	ECustomTeleportType TeleportType;

	UFUNCTION(BlueprintCallable, Category = "Teleport")
	FVector GetTeleportLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Teleport")
	FRotator GetTeleportRotation() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* DirectionArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* IconBillboard;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};