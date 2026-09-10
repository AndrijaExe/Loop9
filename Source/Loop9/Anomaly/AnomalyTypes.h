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
	LoopNumber UMETA(DisplayName = "Loop Number"),
	/** 1.1: a figure standing with its back turned; never moves, vanishes when approached or looked at twice. */
	Watcher UMETA(DisplayName = "Watcher"),
	/** 1.1: an object that drifts a centimetre a second while the player is on the floor. */
	Creep UMETA(DisplayName = "Creep"),
};

inline TArray<ELoopAnomalyType> AllLoopAnomalyTypes()
{
	return {
		ELoopAnomalyType::Hide,
		ELoopAnomalyType::Move,
		ELoopAnomalyType::Light,
		ELoopAnomalyType::Audio,
		ELoopAnomalyType::Text,
		ELoopAnomalyType::DoorLock,
		ELoopAnomalyType::Pursuer,
		ELoopAnomalyType::Scale,
		ELoopAnomalyType::PhantomMessage,
		ELoopAnomalyType::LoopNumber,
		ELoopAnomalyType::Watcher,
		ELoopAnomalyType::Creep,
	};
}

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
	case ELoopAnomalyType::LoopNumber: return TEXT("LoopNumberAnomaly");
	case ELoopAnomalyType::Watcher: return TEXT("WatcherAnomaly");
	case ELoopAnomalyType::Creep: return TEXT("CreepAnomaly");
	default: return TEXT("UnknownAnomaly");
	}
}
