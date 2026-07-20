# Prompt for the Unreal MCP agent

Copy everything below into the AI session that has Unreal Engine 5.8 MCP access.

---

You are finishing the Unreal Editor side of Loop 9 elevator and ending
cinematics.

Repository:
`/home/andrija/Desktop/Loop9/Game/Loop9` on Linux, or the equivalent cloned
project path on the Windows PC.

Steam App ID: `4982260`.

## Rules

1. Pull the latest `main` before editing.
2. Open `Loop9.uproject` in Unreal Engine 5.8 and compile the project.
3. Use Unreal MCP/editor operations for `.uasset`, map, Blueprint and Sequencer
   work. Do not binary-edit `.uasset` files.
4. Do not replace the C++ architecture unless compilation proves a concrete
   error. If compilation fails, make the smallest source fix, rebuild and report
   the exact error and fix.
5. C++ owns gameplay state, input locking, doors, fades, teleport, achievements,
   telemetry and watchdog fallbacks. Sequencer owns only camera, lighting,
   audio and cosmetic prop animation.
6. Never put loop mutation, teleport, achievement, save, level load or input
   restore solely in a Sequencer Event Track.
7. Save every changed asset and map, test in Standalone Game, then commit and
   push the finished editor assets.

Read first:

- `CINEMATIC_SEQUENCE_PLAN.md`
- `RELEASE_CHECKLIST.md`
- `Source/Loop9/Interaction/LoopElevatorTransitionDirector.h/.cpp`
- `Source/Loop9/Interaction/LiftButton.h/.cpp`
- `Source/Loop9/LiftDoorWing.h/.cpp`
- `Source/Loop9/Subsystems/LoopEndingPresenterSubsystem.h/.cpp`
- `Source/Loop9/Loop9GameMode.h`

## Existing C++ contract

The latest source provides:

- `ALoopElevatorTransitionDirector`
- per-button `TransitionDirector`
- per-button `TransitionDoorWings`
- per-button `TransitionSequenceOverride`
- director `LitElevatorArrivalPoint`
- director `ArrivalDoorWings`
- optional director fallback `TransitionSequence`
- C++ door close/open completion delegates
- decision deduplication and deferred world commit
- input/pause/interaction lock
- 2.5 s travel delay after source doors close
- fade + camera cut during hidden teleport
- deterministic arrival in the lit elevator
- missing-asset and timeout fallbacks
- optional `ALoop9GameMode::EndingSequences` map
- ending sequence finished/stopped/watchdog → existing ending widget fallback

Do not duplicate these systems in Blueprint.

## Task A — compile and validate classes

1. Regenerate project files if Unreal requests it.
2. Compile Development Editor.
3. Confirm these classes appear:
   - `Loop Elevator Transition Director`
   - `Lift Button`
   - `Lift Door Wing`
4. Confirm `BP_Loop9GameMode` exposes `Ending Sequences`.
5. If Live Coding cannot reflect new UPROPERTY fields, close the editor, do a
   full rebuild and reopen it.

## Task B — configure the elevator transition in FullOfficeMap

Open `/Game/MyStuff/Maps/FullOfficeMap`.

1. Create Blueprint child:
   `/Game/MyStuff/Blueprints/Cinematics/BP_LoopElevatorTransitionDirector`
   based on `ALoopElevatorTransitionDirector`.
2. Place exactly one instance in `FullOfficeMap`, actor label:
   `ElevatorTransitionDirector`.
3. Set timing defaults:
   - Door Close Timeout: `3.0`
   - Travel Duration: `2.5`
   - Fade Duration: `0.25`
   - Door Open Timeout: `3.0`
4. Identify the elevator that is visibly lit. Place or duplicate a
   `BP_TeleportPoint` inside that cabin:
   - label: `TP_LitElevatorArrival`
   - position: centered where the player capsule safely fits
   - rotation: player looks directly toward the doors/office
   - keep clear of door collision and floor penetration
5. Assign `TP_LitElevatorArrival` to the director's
   `Lit Elevator Arrival Point`.
6. Identify the exact two door-wing actors that must open from inside the lit
   arrival cabin. Assign only those to director `Arrival Door Wings`.
7. There are many lift-wing actors in the map. Do not assign all of them and do
   not guess by class alone; verify each selected actor visually.

## Task C — configure both elevator buttons

Find the two button instances used by gameplay:

- lit/reset elevator button (`ELiftButtonType::Reset`)
- dark/increment elevator button (`ELiftButtonType::Increment`)

For each button:

1. Assign the placed `ElevatorTransitionDirector` to `Transition Director`.
2. Assign only that selected elevator's visible closing door pair to
   `Transition Door Wings`.
3. Verify both wings have correct `Movement Direction`, `Travel Distance` and
   `Movement Speed`.
4. Make sure their closed transform at BeginPlay is correct and that existing
   Blueprint logic does not independently teleport or commit a loop decision.
5. Existing `OnInteracted` may keep button-light/audio feedback, but remove any
   conflicting teleport, loop advance/reset or duplicate door orchestration.

## Task D — create two elevator source sequences

Create:

- `/Game/MyStuff/Cinematics/Elevator/LS_Elevator_Lit`
- `/Game/MyStuff/Cinematics/Elevator/LS_Elevator_Dark`

Each sequence should be approximately 5 seconds, but its Camera Cut should cover
only the visible pre-blackout/door-closing beat (roughly 0–1.8 s). C++ controls
the blackout and arrival.

For each sequence:

1. Add a Cine Camera at believable first-person eye height inside/at the chosen
   elevator.
2. Frame the selected doors naturally; use a very small 0.25–0.4 s look/push
   toward the center, not a dramatic third-person shot.
3. Add button click and door-close audio if appropriate.
4. Optionally add subtle fluorescent flicker or tiny camera vibration.
5. Do not animate door transforms in Sequencer; C++ drives `ALiftDoorWing`.
6. Do not add teleport, loop, achievement or input events.
7. Ensure the camera cut ends cleanly before/at blackout so control returns to
   the player camera after teleport.

Assign:

- lit button `Transition Sequence Override` → `LS_Elevator_Lit`
- dark button `Transition Sequence Override` → `LS_Elevator_Dark`

Optional: create a shared audio-only fallback sequence and assign it to the
director `Transition Sequence`, but button overrides take precedence.

## Task E — elevator Standalone QA

Test in Standalone Game, not only PIE:

1. Choose lit elevator when an anomaly exists.
2. Choose dark elevator when no anomaly exists.
3. Test both wrong choices/reset paths.
4. Spam Interact during the transition: only one decision may register.
5. Press Pause during transition: menu must not open.
6. Verify sequence:
   - chosen doors close;
   - blackout starts only after close or timeout;
   - approximately 2.5 s travel follows;
   - player arrives inside the lit elevator for both choices;
   - lit arrival doors open;
   - movement/look/interact return;
   - prompt does not remain stale.
7. Temporarily clear a sequence reference and verify fallback still completes.
8. Temporarily clear the arrival point and verify legacy Exit teleport fallback
   completes without trapping the player.
9. Reach loop 10 and verify the elevator arrival completes before ending
   presentation starts.

## Task F — create six ending sequences

Create these assets:

- `/Game/MyStuff/Cinematics/Endings/LS_Ending_EscapeTogether`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_ObedientFool`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_ColdBetrayal`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_ParanoidSurvivor`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_MergedMemory`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_TheReplacement`

Keep each sequence 3–8 seconds. The real-time C++ watchdog derives its deadline
from the authored sequence duration plus grace. Use existing environment, props,
camera, lighting and sound; do not require a newly animated human character.

Suggested direction:

- Escape Together: doors open into strong exterior/white light; two shadows or
  a second set of footsteps; hopeful but unsettling.
- Obedient Fool: camera approaches phone/terminal while lights switch off
  behind; end on one lit control.
- Cold Betrayal: elevator stops but doors refuse to open; cold/red light,
  disconnected line and one metal impact.
- Paranoid Survivor: camera turns away from the phone toward the exit; the phone
  rings again on the final black frame.
- Merged Memory: duplicated phone/monitor position through flicker, layered
  reflection/light and a slow push toward white.
- The Replacement: camera settles at Dragojlo's workstation, phone rings, view
  drops toward receiver; existing ending widget and replacement terminal follow.

For every sequence:

1. Include a Camera Cut track.
2. Restore temporary actor/light state at completion where appropriate
   (`When Finished: Restore State`) so replay/testing does not contaminate map
   state.
3. Do not show or create ending widgets in Sequencer.
4. Do not call achievements, telemetry, OpenLevel or ReturnToMainMenu.
5. Let C++ `OnFinished` display the existing mapped ending widget.

## Task G — bind ending sequences

Open `/Game/MyStuff/Blueprints/GameMode/BP_Loop9GameMode`.

Populate `Ending Sequences`:

- EscapeTogether → `LS_Ending_EscapeTogether`
- ObedientFool → `LS_Ending_ObedientFool`
- ColdBetrayal → `LS_Ending_ColdBetrayal`
- ParanoidSurvivor → `LS_Ending_ParanoidSurvivor`
- MergedMemory → `LS_Ending_MergedMemory`
- TheReplacement → `LS_Ending_TheReplacement`

Do not alter the existing `Ending Widget Classes` mapping.

## Task H — ending QA

1. Force/test all six evaluator outcomes.
2. For each ending verify:
   - input locks once;
   - sequence plays once;
   - achievement and telemetry fire once;
   - sequence completion fades to existing ending widget;
   - Continue returns to Main Menu;
   - The Replacement still opens its terminal after its ending widget;
   - missing sequence falls back to the old 2 s fade/widget flow;
   - stopping a sequence manually still reaches the widget;
   - no camera, input, timer or spawned LevelSequenceActor remains afterward.
3. Test one ending in Shipping configuration or a packaged Development build.

## Task I — save and handoff

1. Save:
   - `FullOfficeMap`
   - both button instances/Blueprints if changed
   - `BP_LoopElevatorTransitionDirector`
   - `BP_Loop9GameMode`
   - all eight Level Sequence assets
2. Run Map Check and fix new errors.
3. Report exact actor bindings and any unresolved warning.
4. Commit only intended source/editor assets.
5. Push to `origin/main`.

Final report format:

- Compile result
- Assets created
- FullOfficeMap actors/references assigned
- Elevator QA matrix
- Ending QA matrix
- Files committed
- Commit hash and push result
- Remaining manual visual-polish items

---
