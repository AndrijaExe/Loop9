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
  0.45 like a rare shock). Put it on an empty anchor actor where the figure
  should stand; on activation it spawns `FigureClass` at the anchor, rotated so
  its back faces the player. Default `AnomalyObjectKind` is
  "a man standing with his back turned"; author `AnomalyZone` per placement.
- Vanishes (manifestation only; the floor stays anomalous) when the player is
  within `VanishDistance` (260 cm), on the second look after looking away, after
  `MaxContinuousLookSeconds` (6 s) of staring, or after `MaxLifetimeSeconds`.
  A gap in sight shorter than `MinLookAwaySeconds` (0.5 s) — a doorframe or a
  chair crossing the visibility trace — is neither a look-away nor a restart of
  the stare timer.
  First sight logs `object_inspected` / `figure_back_turned` to the journal.
- No Steam spot achievement (same as `LoopNumber`); `ACH_SPOT_ALL` stays at 9.
  Debug: `Anomaly Watcher` / `Figure` / `BackTurned` filters work.

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
  `the_exit`), then the cutscene: `ALoop9GameMode::EndingSequences[TheExit]`
  Level Sequence if authored, else a 2 s fade straight to the card
  (`THE EXIT` / `TheExitDesc`). `ALoopEndingSceneDirector` refuses this ending
  on purpose (it is the desk scene).
- Dragojlo is silent on that path by design: the stairwell / ground floor have
  no phone. If you place one, put an answerable `AudioAnomaly` on it or leave
  it off.
- Debug: `EndingSetup TheExit` (or `6`) arms the door on the current floor and
  skips the wall check (non-shipping only).

## Editor tasks (Andrija)

Nothing below needs C++; everything is content on `develop`.

1. **Compile** the Windows build first (new files: `Anomaly/WatcherAnomalyComponent.*`,
   `Interaction/Loop9SecretExitDoor.*`). Fix anything the compiler finds and
   push before touching content.
2. **Ringing phones:** on 2–3 `AI_Friend` desk phones add `AudioAnomaly`
   (AnomalySound = `/Game/MyStuff/Sound/Phone/...` ring, `bLooping = true`,
   `AnomalyZone` / `AnomalyObjectKind` = "a desk telephone"). Leave
   `bAnswerable` on. Optionally a short static burst SFX for the pickup.
3. **Watcher:** make `BP_WatcherFigure` (Actor with the pursuer skeletal mesh,
   idle pose, no AI, collision `BlockAll` on the mesh so the visibility trace
   hits it). Place 3–4 empty anchor actors with `WatcherAnomaly`
   (`FigureClass = BP_WatcherFigure`, zone tag per placement: end of corridor,
   behind the printer, meeting-room window…). Orientation is computed at spawn.
4. **Secret ending geometry:** pick one wall segment on the office floor, make
   it its own actor with a Hide `AnomalyComponent` (raise `SelectionWeight`
   modestly, e.g. 1.5, so it shows up but stays rare). Behind it: a short
   stairwell down to a ground-floor stub (one corridor, one street door). Place
   `Loop9SecretExitDoor` as that door (mesh + `OpenSound`/`LockedSound`).
   Make sure no `Loop9ObservationZoneVolume` covers the stairwell, or add one
   named `stairwell` if you want him to be able to mention it later. On the
   door set `HiddenWallActor` to that wall actor — without it any Hide anomaly
   on the floor opens the door.
5. **Cutscene:** author `LS_TheExit` (fade from black, exterior walk on rails,
   house door, interior, close-up on the prop reading **Loop 1**, cut). Assign
   it in `BP_Loop9GameMode → EndingSequences → TheExit`. Until then the fade →
   card path is the fallback and is fine for QA.
6. **Localization:** run GatherText (new keys: `ChatRingingPhoneAnswered`,
   `ChatLineCutAfterRing`, `OpenStreetDoor`, `TheExitTitle`, `TheExitDesc`;
   translations are already in the `.po` files), then compile texts.
7. **Steamworks:** create `ACH_ENDING_THE_EXIT` (hidden, "The Exit — You never
   needed the lift."), change `ACH_ALL_ENDINGS` description to "See every
   ending.", publish. `DefaultEngine.ini` already lists `Achievement_27_Id`.
8. **Render (backend `develop` → deploy to a staging service or the live one):**
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
- The Exit: `EndingSetup TheExit`, walk down, open door → card `THE EXIT`,
  archive shows seven nodes, `ACH_ENDING_THE_EXIT` toast. Without the debug
  command, the door must refuse while the wall is present, and also with the
  wall present but a different Hide anomaly forced (`AnomalyForce <other>`).
  `ACH_PERFECT_RUN` must **not** toast on this ending; `ACH_SILENT_RUN` may.
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
