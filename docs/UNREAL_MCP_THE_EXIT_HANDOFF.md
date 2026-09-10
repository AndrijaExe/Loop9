# Unreal MCP Handoff: Author `LS_TheExit` (1.1 secret ending cutscene)

Copy-paste brief for an AI agent with Unreal Engine MCP access, working on the
`develop` branch of the Loop 9 repository. It follows the same rules as
[UNREAL_MCP_ENDING_SCENES_HANDOFF.md](UNREAL_MCP_ENDING_SCENES_HANDOFF.md);
only the differences are spelled out here.

## Division of labour

- **Andrija (editor, by hand):** all geometry and props — the wall segment with
  the Hide `AnomalyComponent`, the stairwell, the ground-floor stub, the
  `Loop9SecretExitDoor` street door (with `HiddenWallActor` set to that wall),
  the exterior stretch, the house door, the interior and the prop that reads
  **Loop 1**. Also the final look of the spaces (materials, set dressing).
- **Agent (via MCP, in Sequencer):** the Level Sequence itself — cinematic
  camera, camera movement, Camera Cuts, lights, cosmetic audio, the opening
  fade-in. Nothing else.

Do not start the sequence until the geometry exists in the map; a sequence
authored against placeholder actors ends up with broken bindings.

## Objective

Create `/Game/MyStuff/Cinematics/Endings/LS_TheExit` and map it in the active
gameplay GameMode (`BP_Loop9GameMode → EndingSequences → TheExit`). Until the
mapping exists the runtime falls back to a 2 s fade → `THE EXIT` card, which is
fine for QA and must keep working.

## How the runtime plays it (read before authoring)

- `ALoop9SecretExitDoor::TryInteract` → `ULoopManagerSubsystem::TryTriggerSecretExitEnding(HiddenWallActor)`
  → `ULoopEndingPresenterSubsystem::TriggerForcedEnding(TheExit)`. Input is
  locked, then `EndingSequences[TheExit]` is async-loaded and played with
  `ULevelSequencePlayer::CreateLevelSequencePlayer` **in the live
  `FullOfficeMap` world**. There is no level travel: the exterior, the house
  door and the interior must exist in that map (a streamed sub-level or a
  far-away area both work).
- `ALoopEndingSceneDirector` refuses `TheExit` on purpose; the desk scene never
  runs for this ending.
- When the sequence finishes (or its watchdog fires at duration + 5 s) the
  presenter does its own 0.75 s fade to black and shows the card. **Do not add
  a closing Camera Fade track.** Leave the last 10–15 frames visually stable.
- The player is standing at the street door when playback starts. The
  presenter does not fade before the sequence, so the opening must handle the
  cut from gameplay: the door's `OnExitAccepted` Blueprint event may start a
  camera fade to black and hold it, and the sequence's own Fade track (frames
  0–30 only) brings the picture back. This is the one place a Fade track is
  allowed.
- Any length is fine for the watchdog. Target **20–30 s at 30 fps** (600–900
  frames); the six lift endings are 5 s mini-scenes, this one is a walk.

## Storyboard (from ROADMAP_1_1 §5)

Shots at 30 fps, one cinematic camera, one Camera Cut track covering the whole
range. Use hard cuts between shots; movement inside a shot is slow and on
rails (Rail/Spline or keyed transforms, eased).

| Shot | Frames | Content |
|---|---|---|
| 1 | 0–30 | Fade in from black on the street, night, the office door behind camera. |
| 2 | 30–240 | Exterior walk on rails, forward at walking pace, camera at eye height, slight hand-held sway (≤ 2° / ≤ 5 cm). One practical light ahead (street lamp or the house porch). |
| 3 | 240–330 | The house door. Camera settles 1–1.5 m from it; the door opens (possessable door actor, eased rotation over ~40 frames); warm interior light spills out. |
| 4 | 330–540 | Interior, on rails through the hall to the room with the prop. Keep it dim; one lamp. |
| 5 | 540–660 | Close-up on the prop reading **Loop 1**. Slow 20–40 cm push, 35–50 mm, shallow DoF so the text is the only sharp thing. |
| 6 | 660–end | Hold on the text, stable, for the presenter's fade. Cut. |

Audio: exterior ambience, footsteps matching shot 2/4 pace, one door sound for
shot 3, silence on shot 5/6. Use existing project audio; report anything
missing instead of importing external content. No Dragojlo voice — the point of
this ending is that he was never asked.

Photosensitivity: no strobing, no white flashes, the fade-in is the only
brightness ramp.

## Preflight

1. Pull `develop`, open `Loop9.uproject` in UE 5.8, compile the editor target
   (new C++ on `develop`: `WatcherAnomalyComponent`, `Loop9SecretExitDoor`,
   `DragojloMemory`, `Loop9DragojloMemorySubsystem`).
2. Start the MCP server in the editor console: `ModelContextProtocol.StartServer`
   (it is deliberately not auto-started; see
   `Config/DefaultEditorPerProjectUserSettings.ini`). Endpoint:
   `http://127.0.0.1:8000/mcp`.
3. Open `/Game/MyStuff/Maps/FullOfficeMap`, locate by inspection (never by
   guessed names): the wall actor with the Hide component, the
   `Loop9SecretExitDoor` instance, the stairwell, the exterior, the house door
   actor, the interior, the **Loop 1** prop.
4. Confirm the geometry is finished enough to frame. If not, stop and report
   what is missing — do not author around placeholders.
5. Screenshot the six framings before keying anything.

## Mapping and QA

1. Add the `TheExit` key in `EndingSequences` of the active GameMode
   Blueprint; compile, save. Verify no other key points to `LS_TheExit`.
2. QA path: `EndingSetup TheExit` (non-shipping) arms the door on the current
   floor and skips the wall check. Walk down, open the door.
3. Verify in Standalone, not only Sequencer preview:
   - `OnExitAccepted` → black → sequence fades in on the street
   - input locked, sequence ends naturally, one presenter fade, `THE EXIT`
     card, Continue → `MainMenu`
   - `ACH_ENDING_THE_EXIT` toast; **no** `ACH_PERFECT_RUN` toast
   - without `EndingSetup`, the door refuses while the wall is present, and
     also with a different Hide anomaly forced (`AnomalyForce <other>`)
   - temporarily remove the mapping → fade → card fallback still works; restore
4. Reopen the project and check `LS_TheExit` has no red/missing bindings.

## Source control

Save only `LS_TheExit`, the GameMode Blueprint and `FullOfficeMap` (if actor
bindings changed). No autosaves, no unrelated resaves. Commit as
`AndrijaEXE <andrijanstanisic321@gmail.com>` with a message such as
`feat(1.1): author LS_TheExit cutscene`. Push only if asked.

## Completion report

1. asset path, playback range, display rate, shot list with actual frames
2. actors bound (possessables) and spawnables used
3. audio used / missing
4. screenshots of the six shots' first frames and the final frame
5. Standalone QA result for each check above
6. anything left for Andrija (geometry, props, lighting)
