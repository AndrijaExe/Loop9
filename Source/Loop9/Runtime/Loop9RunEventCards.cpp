#include "Runtime/Loop9RunEventCards.h"

#define LOCTEXT_NAMESPACE "Loop9RunEvent"

namespace
{
	FText CallToneBody(ERunEventTone Tone)
	{
		switch (Tone)
		{
		case ERunEventTone::Hostile:
			return LOCTEXT("CallHostile", "You spoke to him harshly. He heard it.");
		case ERunEventTone::Suspicious:
			return LOCTEXT("CallSuspicious", "You doubted the voice on the line.");
		case ERunEventTone::Friendly:
			return LOCTEXT("CallFriendly", "You treated him with some warmth.");
		default:
			return LOCTEXT("CallNeutral", "You called him. Neither of you gave much away.");
		}
	}

	FText CallExtra(const FRunEvent& Event)
	{
		const bool bTwice = Event.Count >= 2;
		if (bTwice && Event.DependencyDelta > 0)
		{
			return LOCTEXT("CallExtraTwiceAsk", "You called twice and asked him to decide.");
		}
		if (bTwice && Event.DependencyDelta < 0)
		{
			return LOCTEXT("CallExtraTwiceKept", "You called twice and kept the choice yourself.");
		}
		if (bTwice)
		{
			return LOCTEXT("CallExtraTwice", "You called twice on this floor.");
		}
		if (Event.DependencyDelta > 0)
		{
			return LOCTEXT("CallExtraAskHim", "You asked him to decide.");
		}
		if (Event.DependencyDelta < 0)
		{
			return LOCTEXT("CallExtraKeptChoice", "You kept the choice yourself.");
		}
		return FText::GetEmpty();
	}

	FText JoinBody(const FText& First, const FText& Second)
	{
		if (Second.IsEmpty())
		{
			return First;
		}
		return FText::Format(LOCTEXT("CallBodyJoin", "{0} {1}"), First, Second);
	}

	FText LoopTitle(const FText& Format, int32 LoopIndex)
	{
		return FText::Format(Format, FText::AsNumber(LoopIndex));
	}
}

FText Loop9RunEventCards::EndingTitle(ELoopEndingType EndingType)
{
	switch (EndingType)
	{
	case ELoopEndingType::EscapeTogether:
		return NSLOCTEXT("Loop9Endings", "EscapeTogetherTitle", "ESCAPE TOGETHER");
	case ELoopEndingType::ObedientFool:
		return NSLOCTEXT("Loop9Endings", "ObedientFoolTitle", "OBEDIENT FOOL");
	case ELoopEndingType::ColdBetrayal:
		return NSLOCTEXT("Loop9Endings", "ColdBetrayalTitle", "COLD BETRAYAL");
	case ELoopEndingType::MergedMemory:
		return NSLOCTEXT("Loop9Endings", "MergedMemoryTitle", "MERGED MEMORY");
	case ELoopEndingType::TheReplacement:
		return NSLOCTEXT("Loop9Endings", "TheReplacementTitle", "THE REPLACEMENT");
	case ELoopEndingType::ParanoidSurvivor:
	default:
		return NSLOCTEXT("Loop9Endings", "ParanoidSurvivorTitle", "PARANOID SURVIVOR");
	}
}

FLinearColor Loop9RunEventCards::RingColor(const FRunEvent& Event)
{
	if (Event.Type != ERunEventType::Call)
	{
		return FLinearColor(0.50f, 0.80f, 1.00f);
	}

	switch (Event.Tone)
	{
	case ERunEventTone::Friendly:
		return FLinearColor(0.35f, 0.85f, 0.45f);
	case ERunEventTone::Hostile:
		return FLinearColor(0.90f, 0.25f, 0.22f);
	case ERunEventTone::Suspicious:
		return FLinearColor(0.95f, 0.85f, 0.25f);
	default:
		return FLinearColor(0.50f, 0.80f, 1.00f);
	}
}

FRunEventCard Loop9RunEventCards::Build(const FRunEvent& Event)
{
	FRunEventCard Card;
	Card.Type = Event.Type;
	Card.Tone = Event.Tone;
	Card.LoopIndex = Event.LoopIndex;
	Card.Count = FMath::Max(Event.Count, 1);
	Card.RingColor = RingColor(Event);

	switch (Event.Type)
	{
	case ERunEventType::Call:
		Card.Title = LoopTitle(
			Card.Count >= 2
				? LOCTEXT("CallTitleTwice", "LOOP {0} · TWO CALLS")
				: LOCTEXT("CallTitle", "LOOP {0} · CALL"),
			Event.LoopIndex);
		Card.Body = JoinBody(CallToneBody(Event.Tone), CallExtra(Event));
		break;

	case ERunEventType::CorrectLift:
		Card.Title = LoopTitle(LOCTEXT("CorrectLiftTitle", "LOOP {0} · RIGHT ELEVATOR"), Event.LoopIndex);
		Card.Body = Event.bAnomalyExisted
			? LOCTEXT("CorrectLiftAnomaly", "You took the right elevator. The floor was already wrong.")
			: LOCTEXT("CorrectLiftClean", "You took the right elevator. The floor was clean.");
		break;

	case ERunEventType::WrongLift:
		Card.Title = LoopTitle(LOCTEXT("WrongLiftTitle", "LOOP {0} · WRONG ELEVATOR"), Event.LoopIndex);
		Card.Body = Event.bAnomalyExisted
			? LOCTEXT("WrongLiftAnomaly", "You chose the wrong elevator. The floor was already lying.")
			: LOCTEXT("WrongLiftClean", "You chose the wrong elevator. The floor was clean.");
		break;

	case ERunEventType::Ending:
		Card.Title = EndingTitle(Event.EndingType);
		Card.Body = LOCTEXT("EndingBody", "This is how the shift ended.");
		break;
	}

	return Card;
}

TArray<FRunEventCard> Loop9RunEventCards::BuildAll(const TArray<FRunEvent>& Events)
{
	TArray<FRunEventCard> Cards;
	Cards.Reserve(Events.Num());
	for (const FRunEvent& Event : Events)
	{
		Cards.Add(Build(Event));
	}
	return Cards;
}

#undef LOCTEXT_NAMESPACE
