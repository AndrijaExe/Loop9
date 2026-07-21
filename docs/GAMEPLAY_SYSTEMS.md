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

- player messages (keyword / phrase heuristics, multi-language)
- AI-diagnosed `[STATE]KINDNESS` / `SUSPICION` deltas from chat replies
- elevator correctness
- AI interaction counts
- periodic stability decay

## Ending evaluation

Evaluator: `FLoopEndingEvaluator` in `Loop/LoopEndingEvaluator.cpp`.

Relationship endings require at least **3** AI interactions. Otherwise the evaluator returns `ParanoidSurvivor`.

Priority sketch (first matching branch wins after the interaction gate):

1. **The Replacement** — high trust/kindness/cooperation/dependency, lower AI stability, ≥11 AI chats
2. **Merged Memory** — humane/cooperative but unstable AI, ≥9 advances, ≥6 AI chats
3. **Cold Betrayal** — trusts / follows while treating him poorly
4. **Obedient Fool** — high dependency, low suspicion
5. **Escape Together** — cooperative, warm, not overly dependent, stable AI
6. **Paranoid Survivor** — low interaction, high suspicion, low trust, or fallback

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
