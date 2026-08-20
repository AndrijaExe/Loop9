# Anomalies

Loop 9 has **nine** anomaly types (`ELoopAnomalyType` in `Anomaly/AnomalyTypes.h`). There is no Clock anomaly.

## Types

| Enum | Label helper | Typical component |
|---|---|---|
| Hide | `HideAnomaly` | hide-object component |
| Move | `MoveAnomaly` | moved-object component |
| Light | `LightFlickerAnomaly` | light flicker |
| Audio | `AudioAnomaly` | audio anomaly |
| Text | `TextAnomaly` | text spawn / material-swap text variants |
| DoorLock | `DoorLockAnomaly` | door lock state |
| Pursuer | `PursuerAnomaly` | pursuer AI anomaly |
| Scale | `ScaleAnomaly` | wrong-sized object |
| PhantomMessage | `PhantomMessageAnomaly` | chat message the player never sent |

`MaterialSwapAnomalyComponent` currently reports type `Text` and is used for material/text visual variants.

## Manager lifecycle

`UAnomalyManager` (Game Instance subsystem):

1. Components register / unregister themselves.
2. `TriggerRandomAnomalies` / loop generation selects candidates for the next loop.
3. `UpdateLoopAnomalyTracking` builds:
   - `CurrentLoopAnomalyKey`
   - `CurrentLoopAnomalyContext`
   - `bCurrentLoopAnomalyRepeat`
   - `CurrentLoopAnomalyZone` / `CurrentLoopAnomalyObjectKind`, from the authored
     fields of one active component chosen by `Loop9RuntimePolicies::SelectAnomalyDetail`
4. Context is sent to the backend chat pipeline as `anomaly_context` / `anomaly_key` /
   `repeat_anomaly`, plus `anomaly_detail` when the zone or object kind is authored.
   A clean floor sends `anomaly_key` as `"none"`; the backend reads that sentinel
   as no anomaly, so do not repurpose the string.
5. `ResetAllAnomalies` clears active state when a loop resets or needs a clean floor.

Loop 1 is intended to stay clean so the player can learn the baseline.

## Authoring rules

1. Put anomaly components on actors in `FullOfficeMap` (or Blueprint children).
2. Prefer reusable Actor Components + Interfaces over one-off Blueprint logic.
3. Ensure components register with `UAnomalyManager` on begin play and unregister on end play.
4. For MaterialSwap variants, keep texture/material references intentional; tracked MaterialSwap content is large.
5. Never rely on Sequencer alone to activate or deactivate anomalies.
6. Fill `AnomalyZone` and `AnomalyObjectKind` (details panel, **Anomaly > AI Context**).
   See [AI context tagging](#ai-context-tagging).

## AI context tagging

Two string fields on every anomaly component decide how specific Dragojlo can be.
Without them he admits he cannot tell where the anomaly is; with them he can send
the player to the right part of the floor without naming the item they must find.

| Field | Example | Rule |
|---|---|---|
| `AnomalyZone` | `the north corridor` | Coarse landmark the player recognises on screen. Empty for placeless anomalies. |
| `AnomalyObjectKind` | `a ceiling light panel` | Category noun. Never an actor name. |

1. Write both in **English**. The model translates into the player's language; the
   strings stay out of the PO files on purpose.
2. Use what the player sees, not level vocabulary: `the copier alcove`, not `Room_B_03`.
3. Never an asset or actor name. `SM_Lamp_03` reaching the chat breaks the fiction.
4. Leave `AnomalyZone` empty for anomalies with no place, above all `PhantomMessage`.
   The backend then offers only the kind and forbids naming a place.
5. Keep each under 48 characters; the backend truncates past that.
6. Do not describe the anomaly itself. `a wall clock` is right, `a missing wall clock`
   gives the answer away.
7. Untagged components are skipped rather than blocking a tagged one on the same
   floor, so the level can be tagged a few anomalies at a time.

## Debug console commands (non-Shipping)

Bound on `ALoop9PlayerController` and compiled out of Shipping:

| Command | Effect |
|---|---|
| `AnomalyList` | Print registered anomalies |
| `AnomalyReset` | Clear all active anomalies |
| `AnomalyForceAny` | Force one random inactive anomaly |
| `AnomalyForce <filter> [matIndex]` | Force matches by type/class/actor; optional MaterialSwap index |
| `AnomalyHelp` | Print usage |

Filter notes:

- Type labels match exactly (case-insensitive).
- Class/actor partial filters require at least 3 characters.
- Examples: `MaterialSwap`, `Text`, `Move`, `OldMagazine`, `I01`, `F01`, `D01`.

## Achievements tied to anomalies

Spotting achievements unlock on correct lit-elevator calls while the matching type is active. Meta achievement `ACH_SPOT_ALL` requires all nine types across runs (persisted). Details: [`../STEAM_ACHIEVEMENTS.md`](../STEAM_ACHIEVEMENTS.md).
