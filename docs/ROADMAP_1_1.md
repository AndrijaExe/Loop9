# Loop 9 — 1.1 roadmap (`develop` branch)

Scope agreed 08.09.2026. `main` is the live v1.0.5 build; everything here lands
on `develop` and merges into `main` only after a full QA pass on a 1.1 cook.

**Status 08.09.2026: all five items are code-complete on `develop` (client +
backend). Nothing has been compiled on Windows yet.** What is left is editor
work, one compile, GatherText, a cook and live QA — see
[Editor tasks](#editor-tasks-andrija) and [QA](#qa-pass-for-the-11-cook).

| # | Feature | Client | Editor | Backend | Status |
|---|---|---|---|---|---|
| 1 | Dragojlo remembers returning players | `FDragojloMemory`, `ULoop9DragojloMemorySubsystem`, `run_history` on chat | — | `RunHistory`, prompt block, `AI_RUN_HISTORY_ENABLED` | code done |
| 2 | Desk phones ring (Audio anomaly rework) | `UAudioAnomalyComponent::bAnswerable` / `Answer()`, `AAI_Friend` answers the ring, floor-wide line cut in `UAnomalyManager` | put an `AudioAnomaly` component on the desk phones that should ring; optional SFX | — | code done |
| 3 | He calls about the wrong floor | `EDragojloAdviceMode::StaleFloor`, `bStaleFloorUsed`, `previous_anomaly_detail` from `UAnomalyManager` | — | `stale_floor` directive, `AI_COMMITMENT_STALE_FLOOR_ENABLED` / `_CHANCE` | code done |
| 4 | Figure with its back turned | `ELoopAnomalyType::Watcher`, `UWatcherAnomalyComponent` | anchor actors + `FigureClass` Blueprint (pursuer mesh, idle pose) | `WatcherAnomaly` label | code done |
| 5 | Secret ending: ground-floor door → "Loop 1" | `ELoopEndingType::TheExit`, `ALoop9SecretExitDoor`, `ULoopManagerSubsystem::TryTriggerSecretExitEnding`, forced presenter path, achievement, archive, PO | wall Hide anomaly, stairwell, ground-floor stub, door actor, cutscene Level Sequence | `the_exit` label (telemetry + run history) | code done |

## 1. Dragojlo remembers — QA notes

- New install: `Game.ini` has no `DragojloMemory` key → nothing is sent. Verify
  in the log: `Dragojlo memory loaded: runs=0`.
- Finish one run, start a new shift, send three messages: the first three
  requests carry `run_history`, the fourth does not (backend logs / probe).
- Expected voice: one short clause of recognition in the first reply, nothing
  else changes. If he names an ending or says "last game", tighten
  `PromptFactory::runHistoryBlock()`, not the client.
- Cloud: the key rides the same file as `SeenEndings`; a second PC with Steam
  Cloud should load the same counters.
- Kill switch without a cook: `AI_RUN_HISTORY_ENABLED=false` on Render.

## 2. Desk phones ring — how it works

- Any `UAudioAnomalyComponent` **on an `AAI_Friend` actor** with `bAnswerable`
  (default on) is a ringing phone. The Audio anomaly itself is unchanged
  (sound spawned on activation); components on other actors still just play.
- Interact while it rings → `AAI_Friend::AnswerRingingAnomaly`: the sound
  stops (`Answer()`, anomaly stays active so the floor still judges "lit"),
  the chat opens with one canned line (`ChatRingingPhoneAnswered`, local, **no
  backend call**, no message slot spent), the journal gets
  `object_inspected` / `ringing_phone`, and `UAnomalyManager::CutPhoneLineForFloor()`
  is set.
- While the line is cut, every phone on the floor answers `SayToAI` with
  `ChatLineCutAfterRing` and nothing is sent. Distinct text from the Pursuer
  dead line. Cleared on the next floor (`BeginLoopVisit`) and on run reset.
- Editor: pick which desk phones ring by adding an `AudioAnomaly` component to
  those `AI_Friend` instances (AnomalySound = the ring, `bLooping`, zone tags).
  Phones without the component never ring.

## 3. He calls about the wrong floor — how it works

- Client: `UAnomalyManager` keeps `PreviousLoopAnomalyZone/ObjectKind` (moved
  from the judged floor in `BeginLoopVisit`). `AAI_Friend` sends them as
  `previous_anomaly_detail` only when the zone is non-empty, so a clean or
  placeless previous floor never produces the slip.
- Backend: `AdvicePolicy::shouldStaleFloor` runs **before** the withhold path
  (player has not reported a finding yet): loop ≥ 4, previous zone present and
  different from the current one, no place lie spent this run
  (`stale_floor_used`, `location_misdirection_used`, `wrong_lift_used`), not a
  Pursuer floor, then a stable per-floor roll against
  `AI_COMMITMENT_STALE_FLOOR_CHANCE` (default 0.35). Directive
  `stale_floor`, lift `none`, `suggested_zone` = previous place.
- Client marks `bStaleFloorUsed` on that mode; it counts as one lie in
  `FDragojloMemory::LiesTold`. Kill switch without a cook:
  `AI_COMMITMENT_STALE_FLOOR_ENABLED=false`.

## 4. Figure with its back turned — how it works

- `UWatcherAnomalyComponent` (type `Watcher`, label `WatcherAnomaly`, weight
  0.45 like a rare shock). Comes built into `AWatcherAnchor`: place the anchor
  where he stands, turn it so the red arrow points where he looks; on
  activation `FigureClass` spawns at the anchor's transform, no rotation math. Default `AnomalyObjectKind` is
  "a man standing with his back turned"; author `AnomalyZone` per placement.
- Vanishes (manifestation only; the floor stays anomalous) when the player is
  within `VanishDistance` (260 cm), on the second look after looking away, after
  `MaxContinuousLookSeconds` (6 s) of staring, or after `MaxLifetimeSeconds`.
  A gap in sight shorter than `MinLookAwaySeconds` (0.5 s) — a doorframe or a
  chair crossing the visibility trace — is neither a look-away nor a restart of
  the stare timer.
  First sight logs `object_inspected` / `figure_back_turned` to the journal.
- Spot achievement `ACH_SPOT_WATCHER` (hidden); he counts toward `ACH_SPOT_ALL`,
  which needs 12 types from 1.1. Debug: `Anomaly Watcher` / `Figure` /
  `BackTurned` filters work.
- **How he leaves** (11.09.): look away and he is gone at once, quietly; the
  first look back at the empty spot cuts the lights. Walk up to him at any
  speed, or stare too long: 0.7 s signal burst (`PlaySignalBurst`), lights out,
  gone behind it; the approach unlocks `ACH_TOO_CLOSE`. The dark lasts until
  the next loop or `AnomalyReset` (`BlackoutSeconds` 0). Lifetime timeout is
  the only silent exit.

## 5. Secret ending — "Loop 1" — how it works

- `ELoopEndingType::TheExit` is **triggered, never scored**:
  `FLoopEndingEvaluator` does not know it. `ALoop9SecretExitDoor::TryInteract`
  asks `ULoopManagerSubsystem::TryTriggerSecretExitEnding(HiddenWallActor)`,
  which accepts only when the run is live, `CurrentLoop ≥ SecretExitMinLoop`
  (4) and the **Hide** anomaly on the door's `HiddenWallActor` is active (the
  wall is really missing). With `HiddenWallActor` unset any active Hide on the
  floor counts, which is why the editor task below sets it. Otherwise the door
  plays `LockedSound` and stays a door, so clipping through geometry cannot
  award the ending.
- Accepted → `ULoopEndingPresenterSubsystem::TriggerForcedEnding(TheExit)`:
  same pipeline as every ending (archive `RecordEnding`, `ACH_ENDING_THE_EXIT`,
  `ACH_ALL_ENDINGS` now needs seven, telemetry `the_exit`, Dragojlo memory
  `the_exit`), input locked. The door plays `OpenSound` and **never opens**.
  Then, when `BP_Loop9GameMode → TheExitLevel` is set: camera fades to black
  over `TheExitFadeSeconds` (1.5 s), the presenter opens the apartment level,
  `ALoop9TheExitGameMode::BeginPlay` hands back to the presenter, which holds
  black, plays the GameMode's `ExitSequence` (first person: run to the flat's
  door, open, turn to close, turn back — the TV reads **LOOP 1**), then the
  0.75 s fade and the card (`THE EXIT` / `TheExitDesc`), Continue → main
  menu. `TheExitLevel` empty = legacy in-place path (`EndingSequences[TheExit]`
  else 2 s fade → card). `ALoopEndingSceneDirector` refuses this ending on
  purpose (it is the desk scene). Brief for the Sequencer work:
  [UNREAL_MCP_THE_EXIT_HANDOFF.md](UNREAL_MCP_THE_EXIT_HANDOFF.md).
- Dragojlo is silent on that path by design: the stairwell / ground floor have
  no phone. If you place one, put an answerable `AudioAnomaly` on it or leave
  it off.
- Debug: `EndingSetup TheExit` (or `6`) arms the door on the current floor and
  skips the wall check (non-shipping only).

## 6. The ringing phone owns the floor — how it works

- `ULoop9RingingFloorSubsystem` (world subsystem, nothing to place) starts when
  an answerable `AudioAnomaly` on an `AI_Friend` activates. It waits for the
  lit lift's doors to finish opening and for the player to be
  `LiftExitDistanceCm` (230) from `LitElevatorArrivalPoint`, then closes the
  director's `ArrivalDoorWings`, marks the lit lift held (the lit `ALiftButton`
  refuses presses) and blacks out the floor through `ULoop9LightsSubsystem`,
  keeping only the world light nearest the phone (`PhoneLampSearchRadiusCm`,
  600). Pawn lights (flashlight) and lift-button indicators are never touched.
- Picking up: `AAI_Friend::AnswerRingingAnomaly` shows one of six lines at
  random (`PickRingingPhoneLine`), read-only (`UAI_ChatWidget::SetInputLocked`);
  the last three point at the wall / stairs / street door of The Exit without
  saying where. Unlocks `ACH_WRONG_NUMBER`. The line stays cut for the floor as
  before.
- Closing the chat (`CloseChatWidget` with `bRingingMessagePending`) restores
  the lights and reopens the lit lift. The dark lift is never held, so a player
  who refuses to answer can still take the wrong lift. `MaxHoldSeconds` (240)
  is the safety net; a loop change (`GenerateAnomalyForNextLoop`) restores the
  lights and the audio component's reset ends the beat.
- Editor: nothing new beyond section 2, but check that each ringing phone has a
  lamp within 6 m, or the floor goes fully dark (logged as a warning).

## 7. Creep — an object that moves while you watch

- `UCreepAnomalyComponent` (type `Creep`, label `CreepAnomaly`, weight 1.0) on
  the object itself. Starts at the normal spot on activation and drifts at
  `CreepSpeedCmPerSecond` (1.0) toward a tagged `AAnomalyMovePoint`
  (`CreepTargetTag`) or `CreepOffset` (60 cm sideways in the object's axes),
  then stops. `bPauseWhileObserved` (off) only moves it when unseen. Under 5 cm
  of travel is refused like a Move onto its own spot.
- `AnomalyObjectKind` default "an object that is slowly moving on its own".
  Spot achievement `ACH_SPOT_CREEP`. Debug: `AnomalyCreep`, filters `Creep` /
  `Drift` / `SlowMove`.
- Backend `develop` needs the `CreepAnomaly` label in the prompt taxonomy
  (same edit as Watcher/LoopNumber).

## Editor tasks (Andrija)

Nothing below needs C++; everything is content on `develop`.

1. **Compile** the Windows build first (new files: `Anomaly/WatcherAnomalyComponent.*`,
   `Interaction/Loop9SecretExitDoor.*`, `Loop9TheExitGameMode.*`). Fix anything the compiler finds and
   push before touching content.
2. **Ringing phones:** on 2–3 `AI_Friend` desk phones add `AudioAnomaly`
   (AnomalySound = `/Game/MyStuff/Sound/Phone/...` ring, `bLooping = true`,
   `AnomalyZone` / `AnomalyObjectKind` = "a desk telephone"). Leave
   `bAnswerable` on. Optionally a short static burst SFX for the pickup.
3. **Watcher:** make `BP_WatcherFigure` (Actor with `SKM_Urban_Nomad`, the
   pursuer's mesh, idle pose or `ABP_Pursuer_Locomotion`, no AI, collision
   `BlockAll` on the mesh so the visibility trace hits it). Place 3–4
   `WatcherAnchor` actors (or a BP child with `FigureClass` preset), red arrow =
   where he looks, `FigureClass = BP_WatcherFigure`, zone tag per placement:
   end of corridor, behind the printer, meeting-room window…
4. **Secret ending geometry:** pick one wall segment on the office floor, make
   it its own actor with a Hide `AnomalyComponent` (raise `SelectionWeight`
   modestly, e.g. 1.5, so it shows up but stays rare). Behind it: a short
   stairwell down to a ground-floor stub (one corridor, one street door). Place
   `Loop9SecretExitDoor` as that door (mesh + `OpenSound`/`LockedSound`).
   Make sure no `Loop9ObservationZoneVolume` covers the stairwell, or add one
   named `stairwell` if you want him to be able to mention it later. On the
   door set `HiddenWallActor` to that wall actor — without it any Hide anomaly
   on the floor opens the door.
5. **Apartment level:** new map (e.g. `/Game/MyStuff/Maps/TheExitApartment`):
   landing outside the flat's door, the door actor, hall, living room, TV with
   an `ALoopNumberSign` on the screen (`FixedLoopValue = 1`), a desk phone
   prop (static mesh, no `AI_Friend`) for the ring at the end. World Settings →
   GameMode Override = `Loop9TheExitGameMode` (or a BP child). On
   `BP_Loop9GameMode` set `TheExitLevel` to this map. Add the map to
   `MapsToCook` in `DefaultGame.ini`.
6. **Cutscene:** author `LS_TheExit` in the apartment map and set it as the
   apartment GameMode's `ExitSequence`. Until then black → card is the
   fallback and is fine for QA. Geometry is hand work; the Sequencer part is
   an MCP agent task — brief in
   [UNREAL_MCP_THE_EXIT_HANDOFF.md](UNREAL_MCP_THE_EXIT_HANDOFF.md). PIE the
   apartment map directly to preview (the presenter runs it as a preview).
7. **Localization:** run GatherText (new keys: `ChatRingingPhoneAnswered`,
   `ChatLineCutAfterRing`, `OpenStreetDoor`, `TheExitTitle`, `TheExitDesc`;
   translations are already in the `.po` files), then compile texts.
8. **Steamworks:** create `ACH_ENDING_THE_EXIT` (hidden, "The Exit — You never
   needed the lift."), change `ACH_ALL_ENDINGS` description to "See every
   ending.", publish. `DefaultEngine.ini` already lists `Achievement_28_Id`.
9. **Creep:** put `CreepAnomalyComponent` on 3-4 small movable props (a vase,
   a mug, a framed photo) with `CreepOffset` pointing along the desk or shelf,
   or drop tagged `AnomalyMovePoint`s and set `CreepTargetTag`. Give each an
   `AnomalyZone`. Check the owner's mobility is Movable (the component forces
   it, but a static-lit mesh will lose its baked shadow).
10. **Ringing phones:** for every `AI_Friend` that has an `AudioAnomaly`, make
   sure a light actor sits within 6 m; that is the lamp that stays on.
11. **Achievement icons:** author `ACH_SPOT_WATCHER_on`, `ACH_SPOT_CREEP_on`,
   `ACH_WRONG_NUMBER_on`, `ACH_TOO_CLOSE_on` (256x256) in
   `Marketing/Steam/Achievements`, then `py -3 Tools/make_achievement_off_icons.py`
   for the locked variants.
12. **Render (backend `develop` → deploy to a staging service or the live one):**
   `AI_COMMITMENT_STALE_FLOOR_ENABLED=true`, `AI_COMMITMENT_STALE_FLOOR_CHANCE=0.35`,
   `AI_RUN_HISTORY_ENABLED=true`. Both are inert for v1.0.5 clients.

## QA pass for the 1.1 cook

- Ringing floor: `AnomalyPhone` forces the ring. Ring audible, prompt "Answer",
  canned line appears, ring stops, other phones say the line-cut text, no
  backend request in the log, next floor phones work again (or
  `PhoneLineRestore` + `AnomalyPhone` to repeat on the same floor). Take the
  **lit** lift → correct.
- Stale floor: play to floor 4+ with an anomaly on the previous floor, ask
  "where should I look" before reporting anything. Log should show mode
  `stale_floor` at most once per run; he must not name a lift.
- Watcher: spawns with his back turned, disappears on approach / second look,
  floor judges lit. `AnomalyWatcher` (or `AnomalyForce Watcher`) forces it.
- The Exit: `EndingSetup TheExit`, walk down, open door → door sound, no
  door movement, 1.5 s to black, apartment cutscene, card `THE EXIT`,
  archive shows seven nodes, `ACH_ENDING_THE_EXIT` toast. Without the debug
  command, the door must refuse while the wall is present, and also with the
  wall present but a different Hide anomaly forced (`AnomalyForce <other>`).
  `ACH_PERFECT_RUN` must **not** toast on this ending; `ACH_SILENT_RUN` may.
- Watcher: `AnomalyWatcher`, walk up to him at any speed. Screen breaks up for
  under a second, every light goes out and stays out until the next floor, he is
  gone, `ACH_TOO_CLOSE` toast. On another floor look away for half a second: he
  is gone with no effect; look back at the spot: lights out, no burst.
  `AnomalyReset` brings the lights back. He faces the anchor's red arrow (drawn
  at head height).
- Ringing floor: `AnomalyPhone` before leaving the lift. Step out → lit doors
  close, floor dark except one lamp by the phone, lit button does nothing.
  Answer → line shown, nothing can be typed, `ACH_WRONG_NUMBER` toast. Close
  chat → lights back, lit lift opens. Dark lift stays usable throughout. Repeat
  a few times to see different lines (`PhoneLineRestore` first: a phone rings
  once per run, the second `AnomalyPhone` on the same desk is refused).
- Text swaps: `AnomalyForce I01`, `AnomalyMaterial`, `AnomalyForce D01`, `AnomalyForce F01`.
  Newspaper headline column reads the words as the paper's own headline, PC
  magazine back cover shows a "next issue" teaser, the manual's monitor a CRT
  prompt, the HTML book its own title block. No sticker look anywhere; run
  `AnomalyAuditMaterials` once on the loaded floor.
- Creep: `AnomalyCreep`, watch a vase for a minute; it should have moved a
  hand's width. Lit lift → correct, `ACH_SPOT_CREEP`.
- Regression: all six original endings via `EndingSetup 0–5`, Pursuer dead
  line, `run_history` recognition on a second run.

## Merge protocol

1. Feature complete on `develop` → cook a 1.1 candidate → playtest branch on
   Steam (same flow as `valvereview`).
2. QA pass documented in `RELEASE_CHECKLIST.md` (new 1.1 section).
3. Backend first: merge `develop` → `main` in `Loop9_backend`, deploy, verify
   `/api/health`; old clients are unaffected because every new field is optional.
4. Then game: merge `develop` → `main`, upload the cook, set as default.
5. `release/v1.0.5` stays untouched as the rollback build.
