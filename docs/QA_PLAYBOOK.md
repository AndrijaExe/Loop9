# QA Playbook

Release status remains authoritative in [`../RELEASE_CHECKLIST.md`](../RELEASE_CHECKLIST.md). This playbook is the reusable test matrix for developers and QA.

## Builds to test

| Build | Must verify |
|---|---|
| Editor Standalone | Elevators, endings, anomalies, UI |
| Packaged Development | Backend auth/chat against staging or prod |
| Shipping via Steam | Auth, achievements, Cloud, no debug commands |

## Gameplay regression

- [ ] Loop 1 is clean; dark elevator is correct.
- [ ] Wrong elevator decision resets as designed.
- [ ] Lit elevator is correct when an anomaly is active.
- [ ] At least one anomaly from each type can be forced and spotted (`AnomalyForce` in non-Shipping).
- [ ] Repeat anomaly context can unlock Déjà Vu conditions.
- [ ] Chat thinking indicator appears and clears.
- [ ] Long chat wait upgrades to “Still thinking...”.
- [ ] Relationship stats move with kind / hostile / dependent messages.
- [ ] Each of the six endings is reachable. `EndingSetup` fixtures still map
  1:1. After `< 3` chats (hard Paranoid), the evaluator scores all six
  profiles — do one natural Cold / Obedient / Merged / Replacement run, not
  only Escape and Paranoid.
- [ ] Ending sequence missing still shows widget and returns to menu.
- [ ] Elevator soft-lock never occurs (door timeouts force progress).
- [ ] Elevator button cannot start a transition while the player is outside the
  cabin interaction volume.
- [ ] Input remains locked during elevator/ending presentation (move/look/jump/sprint/interact/pause).

## Steam / platform

- [ ] App ID `4982260` active.
- [ ] Steam ticket auth succeeds (`POST /api/auth/steam`).
- [ ] Chat uses `X-Session-Token`.
- [ ] At least one achievement from each group unlocks.
- [ ] `ACH_SPOT_ALL` expects ten anomaly types (`LoopNumber` counts since v1.0.6; the 1.1 `Watcher` does not).
- [ ] Steam Cloud syncs intended config files.
- [ ] Shipping build contains no `steam_appid.txt`, game token, or editor-only content.

## Backend / AI

- [ ] `/readyz` is healthy before playtests.
- [ ] Clean loop guidance does not invent anomalies.
- [ ] One Hide/Light/Phantom context is diagnosed sanely.
- [ ] Neutral / kind / suspicious inputs work in EN/SR/DE/FR/RU.
- [ ] Moderation unsafe input returns in-fiction fallback.
- [ ] Provider fallback path recovers when primary is forced down in staging.
- [ ] Telemetry run-finished returns success/no crash on ending.
- [x] Local production QA with two backend replicas and shared Redis enforces
  exact auth, chat burst, IP daily, player daily/monthly and global daily limits.
- [x] Requests rejected by the global daily quota stop before moderation and AI
  generation provider calls.
- [ ] Production quota/cost alarms and alerts fire before the configured global
  daily ceiling becomes a player-visible outage.
- [ ] On the paid always-on Render plan, the first auth/chat request after a long
  idle period has no wake page or client timeout.

## Performance smoke

- [ ] Office traversal without hitch spikes from idle ticking doors/actors.
- [ ] Elevator transition remains smooth with travel audio.
- [ ] Travel audio stops on normal arrival, timeout, abort, ending handoff and
  world teardown; repeated transitions do not stack loops.
- [ ] No unbounded log spam during chat/auth retries.

## Debug tools (non-Shipping)

```
AnomalyList
AnomalyReset
AnomalyForceAny
AnomalyForce <filter> [matIndex]
AnomalyHelp
```

## Shipping smoke (15 minutes)

1. Install from Steam on a clean machine/account.
2. Launch, change language, start run.
3. Talk to Dragojlo once.
4. Spot one anomaly and take lit elevator.
5. Take dark elevator on a clean loop.
6. Finish or force an ending if using an internal build.
7. Confirm overlay achievement toast if expected.
8. Watch backend logs for auth/chat 5xx and quota errors.
9. Confirm `/readyz`, moderation, Redis and both configured AI providers are
   healthy before promoting the depot.

## Commitment path (flag on only)

Keep `AI_COMMITMENT_ENABLED=false` for release cook until Valve/Steam QA is
closed. When enabling on staging:

1. Neutral run without lies — accurate hints, Escape Together still reachable.
2. Dependent run that receives one wrong location (authored decoy), then
   accusation (`SUSPICION=1`).
3. Full Obedient candidate: after contradiction + surrender, at most one dark
   lift on an active anomaly; verify ending distribution still covers all six.
