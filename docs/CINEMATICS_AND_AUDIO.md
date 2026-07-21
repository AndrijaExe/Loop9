# Cinematics and Audio

## Current architecture (authoritative)

Elevator transitions are fully owned by C++ actor `ALoopElevatorTransitionDirector`.

Ending mini-cinematics still use Level Sequences played by `ULoopEndingPresenterSubsystem`.

## Elevator transition phases

```mermaid
stateDiagram-v2
  [*] --> Idle
  Idle --> ClosingDoors: BeginTransition
  ClosingDoors --> Travelling: source doors closed
  Travelling --> OpeningDoors: commit + teleport
  OpeningDoors --> Idle: doors open / timeout force-open
```

Director responsibilities:

- lock player input and clear interaction prompts
- blend look toward the closing doorway
- play button-press sound immediately
- close source doors with timeout fallback
- start looping travel sound after doors close
- fade, commit elevator decision, teleport to lit arrival
- open arrival doors with force-open timeout
- finish/cancel owned decision IDs safely on EndPlay
- complete or cancel deferred endings correctly

### Audio hooks

| Property | When |
|---|---|
| `ButtonPressSound` | Transition start at the pressed button |
| `TravelSound` | After source doors close; stop/fade on arrival |
| `ButtonPressSoundVolume` / `TravelSoundVolume` | Gain |
| `TravelSoundFadeOutSeconds` | Arrival fade-out |

Set these on `BP_LoopElevatorTransitionDirector`.

## Ending sequences

Six Level Sequences are supported via `ALoop9GameMode::EndingSequences`.

Presenter behavior:

1. Evaluate ending type.
2. Optionally play the mapped Level Sequence.
3. Watchdog based on sequence duration (no artificial 120s hard truncate).
4. Fade, cleanup sequence actor, show ending widget.
5. Replacement ending may route through a terminal widget.
6. Return to main menu on a timer after acknowledgement / failure fallback.

Sequencer should only drive camera, lights, props, and cosmetic audio. It must not own:

- loop mutation
- teleport
- achievements
- save/load
- input restore

## Editor QA checklist

- Lit and dark elevator buttons both complete without soft-lock.
- Abandoned / interrupted transitions restore input.
- Travel sound stops on arrival and on abort.
- Each ending sequence reaches its widget.
- Missing sequence asset still shows the ending widget.
- Deprecated `LS_Elevator_Lit` / `LS_Elevator_Dark` are unreferenced before deletion.

Release polish tasks remain tracked only in [`../RELEASE_CHECKLIST.md`](../RELEASE_CHECKLIST.md).
