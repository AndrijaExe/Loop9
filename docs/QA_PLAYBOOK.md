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
- [ ] Each of the six endings is reachable under its evaluator conditions.
- [ ] Ending sequence missing still shows widget and returns to menu.
- [ ] Elevator soft-lock never occurs (door timeouts force progress).
- [ ] Input remains locked during elevator/ending presentation (move/look/jump/sprint/interact/pause).

## Steam / platform

- [ ] App ID `4982260` active.
- [ ] Steam ticket auth succeeds (`POST /api/auth/steam`).
- [ ] Chat uses `X-Session-Token`.
- [ ] At least one achievement from each group unlocks.
- [ ] `ACH_SPOT_ALL` still expects nine anomaly types.
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

## Performance smoke

- [ ] Office traversal without hitch spikes from idle ticking doors/actors.
- [ ] Elevator transition remains smooth with travel audio.
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
