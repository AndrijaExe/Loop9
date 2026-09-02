# Gameplay Systems

## Core fantasy

Year 2003, government office, end of shift. The second floor repeats across nine observation loops. Spot anomalies, choose the correct elevator, and decide how much to trust Dragojlo on the phone. Relationship state steers one of six endings.

## Loop rules

| Rule | Behavior |
|---|---|
| Starting loop | `CurrentLoop = 1` |
| Clean baseline | Loop 1 is clean; player should take the **dark** elevator |
| Lit elevator | Player claims something changed → `EButtonType::Reset` path in design terms is the anomaly call; implementation uses button types `Increment` / `Reset` on `ALiftButton` |
| Dark elevator | Player claims floor is normal → advance path |
| Wrong call | Resets progress toward floor 1 / loop 1 depending on button semantics and correctness |
| Ending trigger | After advancing such that `CurrentLoop >= 10`, the run finishes |

Exact lit/dark mapping in content:

- **Lit elevator** = “I saw an irregularity” (correct when anomalies are active).
- **Dark elevator** = “Nothing changed” (correct when no anomalies are active).

`ULoopManagerSubsystem` owns `CurrentLoop`, `bGameFinished`, and elevator decision IDs.

## Elevator decision flow

```mermaid
sequenceDiagram
  participant Button as ALiftButton
  participant Director as TransitionDirector
  participant Loop as LoopManager
  participant Rel as Relationship
  Button->>Director: BeginTransition
  Director->>Loop: ResolveElevatorDecision
  Note over Director: Close doors, lock input, fade
  Director->>Loop: CommitElevatorDecision
  Loop->>Rel: RegisterLoopDecision
  Note over Director: Teleport, open arrival doors
  Director->>Loop: FinishElevatorTransition / CompleteDeferredEnding
```

Important:

- `ResolveElevatorDecision` snapshots intent without mutating durable state.
- `CommitElevatorDecision` records achievements/stats and may defer ending UI until doors reopen.
- Direct `AdvanceLoop` / `ResetLoop` Blueprint calls are guarded while a transition is active.

## Relationship state

Owned by `URelationshipSubsystem` (defaults):

| Stat | Default | Role |
|---|---|---|
| Trust | 0.5 | Belief in Dragojlo |
| Kindness | 0.5 | Tone toward him |
| Cooperation | 0.5 | Following guidance / correct play |
| Suspicion | 0.2 | Distrust / hostility |
| Dependency | 0.2 | Asking what to do |
| AI Stability | 1.0 | Degrades with mistakes / pressure |

Updates come from:

- AI-diagnosed `[STATE]KINDNESS` / `SUSPICION` / `DEPENDENCY` deltas from chat replies
- elevator correctness
- silent floor: dependency drops if the player never called before leaving
- AI interaction counts
- periodic stability decay

## Ending evaluation

Evaluator: `FLoopEndingEvaluator` in `Loop/LoopEndingEvaluator.cpp`.

Relationship endings require at least **3** AI interactions. Otherwise the evaluator
returns `ParanoidSurvivor` (player who ignored Dragojlo).

After that gate it **scores all six endings** and picks the nearest profile.
There is no waterfall fallback to Paranoid. Signatures:

1. **Escape Together** — first-run good ending: several chats, kind, not clingy, stable AI
2. **Cold Betrayal** — followed him but treated him poorly (low kindness, still trusted)
3. **Obedient Fool** — asked him to decide often (high dependency, not cruel)
4. **Merged Memory** — stayed humane while the run got messy (low AI stability)
5. **The Replacement** — long clingy run, high dependency, AI already unstable
6. **Paranoid Survivor** — high suspicion / low trust after talking, or the <3-chat gate

`EndingSetup` fixtures in `LoopManagerSubsystem` still map 1:1 onto these.
Automation: `Loop9.Runtime.Endings.EvaluatorProfiles`.

Presentation is owned by `ULoopEndingPresenterSubsystem` (optional Level Sequence → fade → widget → main menu). Replacement ending may show a terminal widget path.

## Achievements

Client hooks live in `ULoop9AchievementsSubsystem`. API names and Steamworks setup are authoritative in [`../STEAM_ACHIEVEMENTS.md`](../STEAM_ACHIEVEMENTS.md) (27 achievements).

## Presentation locking

While elevator transitions or ending presentation are active, gameplay input must stay locked:

- move / look
- jump
- sprint
- interact
- pause
- interaction prompts

Helper: `ALoop9Character::IsGameplayPresentationLocked()`.

## Player-facing tutorial line

Initial phone rule (localized) uses lit/dark elevator wording and states that the first loop is clean. Backend prompts mirror the same vocabulary.

## Dragojlo commitment (per-run)

Structured advice memory lives in `ULoopManagerSubsystem` as
`FDragojloCommitmentState`. It tracks the last server `advice` mode / lift /
zone, whether a location lie was used, whether the player later accused him
(`SUSPICION=1`), whether they surrendered a decision on a withheld reply, and
whether a wrong lift was already spent. Cleared only in `ResetRunState` — not
saved, not Clouded, not stored on the backend.

`UAnomalyManager::SelectDecoyZone()` picks one authored inactive zone that
differs from every active zone and has a matching passive
`ALoop9ObservationZoneVolume`. Pursuer and Phantom placements are never decoy
sources. The subsystem registry supplies these checks without world actor scans.
The client sends `decoy_zone` + `advice_state` with each chat request; the
optional response `advice` object updates commitment without parsing reply
text. Elevator commit compares the pressed button to `LastLiftAdvice`.

Entering the suggested volume after `misdirect_location` records a one-shot
visit and elapsed seconds. A later `SUSPICION=1` unlocks one defensive
`confrontation` response. Ending telemetry sends aggregate advice/visit/follow
counts only—never chat, coordinates, paths, or zone names.

## Bounded observation journal

`ULoop9ObservationJournalSubsystem` owns an advisory per-floor journal. Zone
volumes, successful inspections and door actions, flashlight toggles, verified
pursuer sightings/catches, and validated phone responses emit structured
events. Committed lift results update only fixed run counters because the floor
event buffer resets immediately afterward. Floor history resets with
`GenerateAnomalyForNextLoop`; the fixed run summary survives floors and resets
in `ResetRunState`.

The journal is one-way context for `AAI_Friend` chat requests. Gameplay systems
must never read it to judge the lift, mutate relationships, grant achievements,
choose endings, emit telemetry, spawn anomalies, or select `AdvicePolicy`.

Backend switches: `AI_COMMITMENT_ENABLED` (master),
`AI_COMMITMENT_LOCATION_ENABLED`, `AI_COMMITMENT_WRONG_LIFT_ENABLED`, and
`AI_OBSERVATION_CONTEXT_ENABLED` for observation narration only.
