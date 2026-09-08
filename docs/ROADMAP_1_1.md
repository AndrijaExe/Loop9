# Loop 9 — 1.1 roadmap (`develop` branch)

Scope agreed 08.09.2026. `main` is the live v1.0.5 build; everything here lands
on `develop` and merges into `main` only after a full QA pass on a 1.1 cook.
Order is by risk: things that need no editor work first, the secret ending last.

| # | Feature | Client | Editor | Backend | Status |
|---|---|---|---|---|---|
| 1 | Dragojlo remembers returning players | `FDragojloMemory`, `ULoop9DragojloMemorySubsystem`, `run_history` on chat | — | `RunHistory`, prompt block, `AI_RUN_HISTORY_ENABLED` | code done, needs cook + live QA |
| 2 | Desk phones ring (Audio anomaly rework) | answerable phone interactable, canned line, "low signal" state for the floor | pick which desk phones ring; SFX; widget text | — | not started |
| 3 | He calls about the wrong floor | new `EDragojloAdviceMode` or prompt-only; gate on "previous floor had an anomaly" | — | directive + prompt | not started |
| 4 | Figure with its back turned | new anomaly component reusing `PursuerAnomalyCharacter` spawn/visibility rules, never moves | mesh (Dragojlo's or new), placements per zone | AI label + zone tag | not started |
| 5 | Secret ending: ground-floor door → "Loop 1" | ending enum + evaluator gate, missing-wall anomaly → stairwell, cutscene trigger, archive card, achievement | stairwell geometry, ground floor stub, door, cutscene sequence, house interior with "Loop 1" prop | none (Dragojlo silent on that path) | design below |

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

## 2. Desk phones ring

- Reuse `AudioAnomaly` slot: on an Audio floor, one authored desk phone rings
  instead of (or in addition to) the current cue. Interact = pick up.
- Pickup plays a short canned line (local WAV or on-screen text, **no
  backend call**), then the line cuts. The floor is then "low signal": the
  main phone shows a distinct message (not the Pursuer dead line) and does not
  send. Keep the message key separate: `ChatLowSignal` vs `ChatPursuerNoAnswer`.
- Observation journal: emit `object_inspected` with subject `desk_phone` so the
  backend can talk about it next floor.

## 3. He calls about the wrong floor

- Trigger only when the **previous** floor had an anomaly; otherwise the
  "wrong" advice would point at nothing and read as a bug.
- Backend-driven: a new `AdviceDirective` mode (`stale_floor`) that tells the
  model to describe the previous floor's anomaly zone/object as if it were
  current. Client sends `previous_anomaly_detail` (zone + object of the last
  floor) alongside `anomaly_detail`; policy uses it only in that mode.
- Counts as one lie for `FDragojloMemory::LiesTold`.

## 4. Figure with its back turned

- New `ELoopAnomalyType` or a Pursuer sub-mode; simplest is a new component
  `BackTurnedFigureAnomalyComponent` that places a static character actor in
  one authored zone, facing away, never moving, despawning when the player is
  within N metres or looks away then back.
- Zone/object tags: `AnomalyZone` = the authored zone, `AnomalyObjectKind` =
  "a man standing with his back turned".
- Achievement: extend `ACH_SPOT_ALL` count if it becomes a tenth type.

## 5. Secret ending — "Loop 1"

Design constraints from the 08.09 discussion:

- Reachable only via the Hide anomaly hitting an authored wall segment.
  Behind it: a stairwell, not a fall. Stairs down lead to a ground-floor stub
  with one exterior door.
- Gate: loop ≥ N (proposal: 5) so it cannot short-circuit a first run; and the
  player must have made at least one lift decision on the floor before.
- Dragojlo: if the player calls from the stairwell or ground floor, the phone
  is "low signal" (reuse item 2's state). He does not comment on the exit.
- The cutscene is minimal: fade, exterior walk (camera on rails), house door,
  interior, close-up on one prop reading **Loop 1**, cut to credits.
- Systems touched: `ELoopEndingType` (+1, update `EndingTypeCount` and every
  switch), `FLoopEndingEvaluator` (bypass — this ending is triggered, not
  scored), `ULoop9AchievementsSubsystem` (new `ACH_ENDING_*`, `SeenEndings`
  merge maps), archive/ending widget, `Loop9RuntimePolicies::AllEndingTypes()`,
  localization (`GatherText`), telemetry ending label, `FDragojloMemory`
  ending wire label + backend `RunHistory::ENDINGS`.
- Steamworks: one new achievement, hidden.

## Merge protocol

1. Feature complete on `develop` → cook a 1.1 candidate → playtest branch on
   Steam (same flow as `valvereview`).
2. QA pass documented in `RELEASE_CHECKLIST.md` (new 1.1 section).
3. Backend first: merge `develop` → `main` in `Loop9_backend`, deploy, verify
   `/api/health`; old clients are unaffected because every new field is optional.
4. Then game: merge `develop` → `main`, upload the cook, set as default.
5. `release/v1.0.5` stays untouched as the rollback build.
