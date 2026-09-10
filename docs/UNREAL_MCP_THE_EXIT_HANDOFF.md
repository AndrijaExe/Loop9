# Unreal MCP Handoff: Author `LS_TheExit` (1.1 secret ending cutscene)

Copy-paste brief for an AI agent with Unreal Engine MCP access, working on the
`develop` branch of the Loop 9 repository on Windows. It follows the same rules
as [UNREAL_MCP_ENDING_SCENES_HANDOFF.md](UNREAL_MCP_ENDING_SCENES_HANDOFF.md);
only the differences are spelled out here.

## Division of labour

- **Andrija (editor, by hand):** all geometry and props. In `FullOfficeMap`:
  the wall segment with the Hide `AnomalyComponent`, the stairwell, the
  ground-floor stub, the `Loop9SecretExitDoor` street door (`HiddenWallActor`
  = that wall, `OpenSound` = a door-opening sound). In the new apartment map
  (`/Game/MyStuff/Maps/TheExitApartment` or similar): the landing / street
  side of the flat's door, the door actor, the hall, the living room, the TV
  with an `ALoopNumberSign` on its screen (`FixedLoopValue = 1`, same look as
  the office sign), and a desk phone on a table in the living room (a static
  mesh prop — **not** an `AI_Friend`, nothing interactable in this map).
  World Settings → GameMode Override = `Loop9TheExitGameMode` (or a Blueprint child of it) with `ExitSequence` set
  once the sequence exists. `BP_Loop9GameMode → TheExitLevel` = the apartment
  map; `TheExitFadeSeconds` stays 1.5.
- **Agent (via MCP, in Sequencer):** the Level Sequence — cinematic camera,
  camera movement, Camera Cuts, the door's opening/closing keys, lights,
  cosmetic audio, the opening fade-in. Nothing else.

Do not start the sequence until the apartment geometry exists; a sequence
authored against placeholder actors ends up with broken bindings.

## Objective

Create `/Game/MyStuff/Cinematics/Endings/LS_TheExit` **in the apartment map**
(possessables bind to that level) and assign it to the apartment GameMode's
`ExitSequence`. Until then the runtime holds black briefly and shows the
`THE EXIT` card, which is fine for QA and must keep working.

## How the runtime plays it (read before authoring)

1. Player interacts with the street door → `ULoopManagerSubsystem::TryTriggerSecretExitEnding(HiddenWallActor)`
   accepts → `ULoopEndingPresenterSubsystem::TriggerForcedEnding(TheExit)`:
   archive, `ACH_ENDING_THE_EXIT`, telemetry, Dragojlo memory, **input locked**.
2. The door plays `OpenSound` and does **not** open. The presenter fades the
   camera to black over `TheExitFadeSeconds` (1.5 s), then opens
   `ALoop9GameMode::TheExitLevel`. The office world is torn down; the presenter
   (a GameInstance subsystem) keeps the pending ending.
3. `ALoop9TheExitGameMode::BeginPlay` in the apartment calls
   `ContinueTheExitInLevel`: the player is a spectator (no pawn), input is
   locked, the screen is **held black**, `ExitSequence` is async-loaded and
   played with `ULevelSequencePlayer::CreateLevelSequencePlayer`.
4. The sequence's own Fade track lifts the black. **Its Fade section must be
   set to "When Finished: Keep State"** — with the default Restore State the
   held black snaps back at the last frame instead of the presenter's fade.
5. When the sequence finishes (or its watchdog fires at duration + 5 s) the
   presenter does its own 0.75 s fade to black and shows the card. Do not add
   a closing Camera Fade. Leave the last 10–15 frames visually stable.
6. Continue → `MainMenu`.

`ALoopEndingSceneDirector` refuses `TheExit`; the desk scene never runs. If
`TheExitLevel` is left empty the old in-place path runs (`EndingSequences
[TheExit]` in the office, else fade → card) — that is only a fallback.

Any length works for the watchdog. Target **15–25 s at 30 fps**.

## Storyboard

First person: the game never shows the player's body, so the camera *is* the
player. One cinematic camera, one Camera Cut track for the whole range,
continuous take (no hard cuts). Camera height ~160 cm, 35–50 mm.

| Shot | Frames (30 fps) | Content |
|---|---|---|
| 1 | 0–45 | Fade in from black (Fade track, 1.0 → 0.0, Keep State). Camera outside the flat's door, a few metres back, already moving. |
| 2 | 45–150 | **Running to the door**: forward on rails at running pace (~4 m/s), running bob (vertical ±6–8 cm, roll ±1.5°, ~2.6 Hz), a little lateral sway. Breathing / fast footsteps. |
| 3 | 150–210 | At the door: camera decelerates to ~1 m; the door **opens inward** (possessable door actor, eased yaw over ~35 frames); camera steps through the frame. Door-open sound. |
| 4 | 210–300 | **Turns to close it**: camera yaws ~180° (eased, ~60 frames) to face the door from inside; the door swings shut (~40 frames); latch sound. Breathing slows. |
| 5 | 300–390 | **Turns back to the room**: camera yaws ~180° back (eased). At the end of the turn the TV is centred in frame, reading **LOOP 1** in the office sign's red — the `ALoopNumberSign` on the screen, `FixedLoopValue = 1`. Nothing else in the room moves. Silence, or a faint TV hum. |
| 6 | 390–end | Hold. A very slow 10–20 cm push toward the TV is allowed; no cut. In the last ~1.5 s: key the TV sign's `bEnableFlicker` (bool property track on the `ALoopNumberSign` possessable) to **true** so LOOP 1 stutters once, and the desk phone starts ringing (spatialized audio track at the prop, the office ring sound, ≤ 1.5 s). Neither resolves — the presenter fades out over both. |

Lighting: hall dim, one practical in the living room, the TV screen the
brightest thing in shot 5–6. No strobing, no white flashes — the fade-in is
the only brightness ramp.

Audio: use existing project sounds (door open/close, footsteps, breathing,
the phone ring from `/Game/MyStuff/Sound/Phone/`); report anything missing
rather than importing external content. No Dragojlo voice — the point is that
he was never asked; the ring at the end says he knows where the player lives.

The `bEnableFlicker` key on the sign is the one property key allowed on a
gameplay actor here: it is cosmetic (`ALoopNumberSign` reads it in Tick) and
touches no loop state. `bEnableGlitch` works the same way (LOOP ? cycle) but
is stronger than this moment needs.

## Preflight

1. Pull `develop`, open `Loop9.uproject` in UE 5.8 on Windows, compile the
   editor target. New C++ on `develop`: `WatcherAnomalyComponent`,
   `Loop9SecretExitDoor`, `DragojloMemory`, `Loop9DragojloMemorySubsystem`,
   `Loop9TheExitGameMode`.
2. Start the MCP server in the editor console: `ModelContextProtocol.StartServer`
   (deliberately not auto-started; see
   `Config/DefaultEditorPerProjectUserSettings.ini`). Endpoint
   `http://127.0.0.1:8000/mcp`.
3. Open the apartment map. Locate by inspection (never by guessed names): the
   flat's door actor, the hall, the living room, the TV and its
   `ALoopNumberSign`. Confirm `FixedLoopValue = 1` and the GameMode Override.
4. Confirm the geometry is finished enough to frame. If not, stop and report
   what is missing — do not author around placeholders.
5. Screenshot the six framings before keying anything.

## Mapping and QA

1. Set `ExitSequence` on the apartment GameMode (Blueprint or class default);
   set `TheExitLevel` on `BP_Loop9GameMode`. Compile, save both.
2. **Preview without the office:** PIE the apartment map directly. The
   presenter logs "running as preview", plays the sequence, shows the card.
   Fast iteration loop for framing.
3. **Full path in Standalone:** in the office, `EndingSetup TheExit`
   (non-shipping) arms the door; walk down, open it. Verify:
   - door sound, door does not move, input dead, 1.5 s fade to black
   - no visible frame of the apartment before the sequence's fade-in
   - sequence ends naturally, one presenter fade, `THE EXIT` card, Continue →
     `MainMenu`
   - `ACH_ENDING_THE_EXIT` toast; **no** `ACH_PERFECT_RUN` toast
   - without `EndingSetup`, the door refuses while the wall is present, and
     also with a different Hide anomaly forced (`AnomalyForce <other>`)
4. Fallbacks: clear `ExitSequence` → black → card still works; clear
   `TheExitLevel` → old in-place fade → card still works. Restore both.
5. Reopen the project and check `LS_TheExit` has no red/missing bindings.

## Source control

Save only `LS_TheExit`, the apartment map, the two GameMode Blueprints and
`FullOfficeMap` (if actor bindings changed). No autosaves, no unrelated
resaves. Commit as `AndrijaEXE <andrijanstanisic321@gmail.com>` with a message
such as `feat(1.1): author LS_TheExit cutscene`. Push only if asked.

## Completion report

1. asset paths, playback range, display rate, shot list with actual frames
2. actors bound (possessables) and spawnables used
3. audio used / missing
4. screenshots of the six shots' first frames and the final frame
5. Standalone QA result for each check above
6. anything left for Andrija (geometry, props, lighting)
