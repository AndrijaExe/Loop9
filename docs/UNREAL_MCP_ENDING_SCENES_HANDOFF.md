# Unreal MCP Handoff: Finish the Six Ending Mini-Scenes

This is a copy-paste task brief for an AI agent with Unreal Engine MCP access.
Work in the Loop 9 repository and edit the existing Unreal assets directly.

## Objective

Polish the six existing ending Level Sequences into short, readable 4–6 second
mini-scenes, verify their GameMode mappings, and test that each sequence hands
off to the correct ending widget without changing gameplay logic.

Existing assets:

- `/Game/MyStuff/Cinematics/Endings/LS_Ending_EscapeTogether`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_ObedientFool`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_ColdBetrayal`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_ParanoidSurvivor`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_MergedMemory`
- `/Game/MyStuff/Cinematics/Endings/LS_Ending_TheReplacement`

The runtime player is already implemented in
`ULoopEndingPresenterSubsystem`. `ALoop9GameMode::EndingSequences` is a map from
`ELoopEndingType` to soft Level Sequence references. Missing or broken mappings
fall back to the ending widget, but the goal is to make all six mappings valid.

## Hard constraints

1. Use Unreal MCP to inspect the project and tool schemas before editing.
2. Modify the six existing assets. Do not create duplicate `*_v2` assets.
3. Do not change C++ unless a verified engine integration blocker exists.
4. Do not place gameplay logic in Event Tracks, Level Blueprint, or Sequencer.
5. Sequencer may control only:
   - cinematic camera and Camera Cuts
   - lights
   - cosmetic props
   - cosmetic audio
6. Do not mutate loops, relationships, achievements, saves, teleportation,
   elevator decisions, widgets, or input state from Sequencer.
7. Do not add Camera Fade tracks. The presenter performs its own 0.75-second
   fade after a sequence finishes.
8. The ending starts after the elevator transition has completed and the
   arrival doors have reopened. Do not begin a sequence by snapping those doors
   from open back to closed.
9. Keep the sequences safe for photosensitive players:
   - no rapid strobing
   - at most two deliberate light pulses in `Merged Memory`
   - no full-screen white flashes
10. Use existing project audio where suitable. If a required sound does not
    exist, leave the track empty and report the missing asset instead of
    importing random external content.

## Preflight

1. Pull the latest `main`.
2. Open `Loop9.uproject` in Unreal Engine 5.8 and compile the editor target.
3. Open `/Game/MyStuff/Maps/FullOfficeMap`.
4. Compile and save all loaded Blueprints before changing sequences.
5. Determine the active gameplay GameMode:
   - inspect `FullOfficeMap` World Settings override
   - inspect `/Game/MyStuff/Blueprints/GameMode/BP_Loop9GameMode`
   - inspect `/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode` only if the
     map actually uses it
6. Inspect the `EndingSequences` map and verify one exact mapping per enum:
   - `EscapeTogether`
   - `ObedientFool`
   - `ColdBetrayal`
   - `ParanoidSurvivor`
   - `MergedMemory`
   - `TheReplacement`
7. Open every sequence and record:
   - current playback range and display rate
   - broken or missing bindings
   - current Camera Cut track
   - currently referenced actors/audio
8. Take a viewport screenshot before editing each scene.

Do not assume actor names. Locate the real phone, monitor, chair, elevator,
door-wing, light, and office actors in `FullOfficeMap` through MCP inspection.

## Shared sequence standard

Apply this standard to all six assets:

- display rate: 30 fps
- playback range: preferably 150 frames (5 seconds), allowed range 120–180
  frames (4–6 seconds)
- exactly one active Camera Cut track covering the full playback range
- one cinematic camera per sequence
- use smooth cubic/eased transform keys; no abrupt camera cuts unless the
  storyboard explicitly asks for one
- keep camera movement small: approximately 20–100 cm translation and
  5–35 degrees rotation
- use a 35–50 mm focal length unless the inspected composition needs otherwise
- use spawnables for cinematic-only cameras, lights, text, or duplicate props
- use possessables only for existing map props that must visibly move
- avoid editing shared materials globally; use a dynamic/sequence-safe material
  instance or a spawned Text Render actor for temporary monitor messages
- audio must not loop beyond the sequence range
- leave at least the final 10–15 frames visually stable so the presenter fade
  has a clean transition

If an existing sequence already satisfies part of the brief, preserve it and
make the smallest edit needed.

## Scene 1: Escape Together

Asset: `LS_Ending_EscapeTogether`

Intent: warmth, relief, and evidence that Dragojlo may be leaving too.

Storyboard at 30 fps:

- frames 0–30: camera settles just inside the already-open arrival elevator,
  looking toward the office/exit; keep the initial lighting close to gameplay
- frames 30–90: raise one warm practical/rect light gradually; do not flash;
  reveal a clear path beyond the lift
- frames 60–120: slow 40–70 cm camera push toward the light
- frames 85–115: first soft footstep pair, representing the player
- frames 105–135: second, slightly delayed footstep pair from behind/off-axis,
  representing Dragojlo
- frames 120–140: briefly illuminate or activate the second elevator/adjacent
  indicator if a suitable actor exists
- frames 140–150: hold the warm final composition

Do not force the arrival doors to open again. They are already open when the
ending presenter starts.

Acceptance:

- ending reads as hopeful without dialogue
- second presence is suggested by sound/light, not a new character model
- no actor snaps at frame 0

## Scene 2: Obedient Fool

Asset: `LS_Ending_ObedientFool`

Intent: the player obeyed perfectly and became part of the task.

Storyboard:

- frames 0–30: camera faces Dragojlo's phone/desk from a medium distance
- frames 20–115: slow dolly toward the phone or monitor
- frames 35, 60, 85: switch off up to three background lights one at a time;
  keep the phone/monitor readable
- frames 80–135: show `TASK COMPLETE` on an existing monitor using a safe
  material parameter, existing text actor, or sequence spawnable Text Render
- frames 110–125: play one short phone ring
- frames 125–150: hold on the phone and message

Acceptance:

- foreground remains readable after background lights turn off
- text exists only for the cinematic and does not permanently alter a shared
  material asset
- phone rings once, not as a loop

## Scene 3: Cold Betrayal

Asset: `LS_Ending_ColdBetrayal`

Intent: Dragojlo withdraws help and leaves the player with an unsafe exit.

Storyboard:

- frames 0–30: camera looks through the open lift/doorway into darkness
- frames 20–90: 30–50 cm camera move toward the threshold
- frames 55–80: play a short phone-line disconnect/static cut
- frames 70–115: fade in a low red side/back light, never a full-screen flash
- frames 95–135: if the real door-wing actors are idle and binding is safe,
  close them slowly; otherwise leave them untouched and let darkness/red light
  carry the scene
- frames 135–150: hold the narrow red/dark composition

Acceptance:

- no collision/gameplay event is triggered by the cinematic door movement
- sequence remains valid if door movement is omitted
- audio ends before the sequence ends

## Scene 4: Paranoid Survivor

Asset: `LS_Ending_ParanoidSurvivor`

Intent: the player escapes alone but can no longer trust what is behind them.

Storyboard:

- frames 0–35: camera looks away from the phone toward the exit
- frames 30–95: slow turn of approximately 15–25 degrees toward safety
- frames 85–105: phone rings once from behind/off-screen
- frames 100–140: camera begins turning back only 8–15 degrees
- frames 140–150: cut the motion before the phone/source is revealed

Acceptance:

- phone remains off-screen
- final shot creates an interrupted reveal
- no monster, jump-scare actor, or extra gameplay event is added

## Scene 5: Merged Memory

Asset: `LS_Ending_MergedMemory`

Intent: player and AI memories overlap; space appears to remember two versions
of the same moment.

Storyboard:

- frames 0–30: stable view of the phone/monitor area
- frames 30–105: slow camera approach
- around frames 45 and 75: perform exactly two mild fluorescent intensity
  pulses; never drop from full black to full white
- frames 55–100: show a duplicate phone/monitor position using a spawnable
  duplicate or visibility-controlled cosmetic duplicate; offset it slightly
- frames 65–110: play a second copy of one quiet sound 0.2–0.35 seconds after
  the first
- frames 110–135: remove/fade the duplicate and settle on the real monitor
- frames 135–150: stable final hold

Acceptance:

- maximum two light pulses
- duplicate prop is sequence-owned or restored automatically
- scene reads as spatial overlap, not a rendering bug

## Scene 6: The Replacement

Asset: `LS_Ending_TheReplacement`

Intent: the player is becoming the new operator.

Storyboard:

- frames 0–30: establish Dragojlo's desk and empty chair
- frames 20–115: slow camera move toward the desk
- frames 55–105: rotate the empty chair subtly, approximately 10–20 degrees
- frames 85–115: phone rings once
- frames 95–135: show `NEW OPERATOR CONNECTED` on the monitor using the same
  non-destructive approach as `TASK COMPLETE`
- frames 135–150: hold on the chair/monitor

The presenter automatically shows the normal ending widget and then starts
`WBP_ReplacementTerminal` after three seconds. Do not spawn or trigger the
terminal from Sequencer.

Acceptance:

- chair movement is subtle and physically plausible
- monitor text is readable
- sequence hands off to the existing Replacement widget/terminal flow exactly
  once

## Mapping and integration verification

After editing:

1. Compile and save the active gameplay GameMode Blueprint.
2. Confirm every `EndingSequences` key points to the correct asset and no two
   keys point to the same asset accidentally.
3. Save all six sequences and `FullOfficeMap` if map actor bindings changed.
4. Reopen each sequence and verify there are no red/missing bindings.
5. Verify no ending sequence references:
   - `ULoopManagerSubsystem`
   - elevator decision functions
   - achievement functions
   - save/config functions
   - widget creation
6. Use Reference Viewer to confirm the sequences are referenced by the active
   GameMode Blueprint.

## QA

Test each ending separately. Prefer an existing debug/force-ending mechanism.
If none exists, temporarily adjust runtime relationship values only in a local
test session and revert them before saving/committing.

For every ending verify:

- correct sequence plays
- input remains locked during playback
- sequence ends naturally
- presenter performs one smooth fade
- correct ending widget appears
- Continue returns to `MainMenu`
- missing audio does not block playback
- no red/missing Sequencer bindings after reopening the project

Additional checks:

- remove one mapping temporarily and verify widget fallback, then restore it
- stop one sequence early and verify the presenter still reaches the widget
- verify `TheReplacement` opens its terminal only through the presenter
- run all six in Standalone, not only Sequencer preview
- inspect Output Log for warnings/errors

## Source control rules

1. Before editing, record `git status`.
2. Save only intended Level Sequence, GameMode Blueprint, and map assets.
3. Do not commit autosaves, DerivedDataCache, Intermediate, Saved, logs, or
   unrelated Blueprint resaves.
4. After QA, report the exact changed asset paths.
5. Commit with a message such as:
   `feat: polish six ending mini-cinematics`
6. Push only if the user explicitly asks.

## Required completion report

Return:

1. changed assets
2. mapping verified for all six enum values
3. duration and main tracks for each scene
4. audio assets used or missing
5. screenshots for all six final frames
6. Standalone QA result per ending
7. any manual work still required
