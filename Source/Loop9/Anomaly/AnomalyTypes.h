#pragma once

#include "CoreMinimal.h"
#include "AnomalyTypes.generated.h"

UENUM(BlueprintType)
enum class ELoopAnomalyType : uint8
{
	Hide UMETA(DisplayName = "Hide"),
	Move UMETA(DisplayName = "Move"),
	Light UMETA(DisplayName = "Light Flicker"),
	Audio UMETA(DisplayName = "Audio"),
	Text UMETA(DisplayName = "Text Spawn"),
	DoorLock UMETA(DisplayName = "Door Lock"),
	Pursuer UMETA(DisplayName = "Pursuer"),
	Scale UMETA(DisplayName = "Scale"),
	PhantomMessage UMETA(DisplayName = "Phantom Message"),
};

inline FName GetLoopAnomalyTypeLabel(ELoopAnomalyType Type)
{
	switch (Type)
	{
	case ELoopAnomalyType::Hide: return TEXT("HideAnomaly");
	case ELoopAnomalyType::Move: return TEXT("MoveAnomaly");
	case ELoopAnomalyType::Light: return TEXT("LightFlickerAnomaly");
	case ELoopAnomalyType::Audio: return TEXT("AudioAnomaly");
	case ELoopAnomalyType::Text: return TEXT("TextAnomaly");
	case ELoopAnomalyType::DoorLock: return TEXT("DoorLockAnomaly");
	case ELoopAnomalyType::Pursuer: return TEXT("PursuerAnomaly");
	case ELoopAnomalyType::Scale: return TEXT("ScaleAnomaly");
	case ELoopAnomalyType::PhantomMessage: return TEXT("PhantomMessageAnomaly");
	default: return TEXT("UnknownAnomaly");
	}
}
