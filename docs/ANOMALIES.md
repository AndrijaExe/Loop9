# Anomalies

Loop 9 has **ten** anomaly types (`ELoopAnomalyType` in `Anomaly/AnomalyTypes.h`). There is no Clock anomaly. `ACH_SPOT_ALL` still tracks the original nine floor-search types; `LoopNumber` counts for the elevator and for Dragojlo, but has no `ACH_SPOT_*`.

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
| LoopNumber | `LoopNumberAnomaly` | loop counter flickers / turns into `?` |

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

## How often anomalies appear

Two knobs, both in `LoopManagerSubsystem.cpp`:

- `AnomalyChancePerLoop` (currently `0.8`) — odds that a floor past the baseline
  carries at least one anomaly. Loop 1 ignores it and is always clean, because the
  opening phone call promises the player exactly that.
- `ComputeAnomalyTargetCount` — how many anomalies an anomalous floor aims for,
  scaling with loop index and falling AI stability.

Which anomalies fill those slots is a two-stage weighted draw in
`UAnomalyManager::TriggerRandomAnomalies`:

1. A type is drawn using `GetTypeSelectionWeight`. Hide and Text sit at `1.8`
   because they reward searching; Pursuer sits at `0.35` because it replaces the
   search with a chase; LoopNumber sits at `0.55` because the counter is already
   in the player's face.
2. A component inside that type is drawn using its own `SelectionWeight`.
   `MaterialSwapAnomalyComponent` ships at `2.0`, so it wins the Text pool it
   shares with the spawned-note anomaly.
3. `AnomalyProbability` is rolled last and only decides whether that draw counts.
   A lost roll no longer costs the floor an anomaly — the next draw replaces it.

`SelectionWeight` is the safe knob for favouring one placement over another, since
it never changes how full a floor ends up being. Note that a C++ constructor
default only reaches placements that never overrode the value in the editor.

If a floor claimed an anomaly but nothing activated, `ForceActivateAnyAnomaly`
rescues it using the same type weights, and keeps trying past any component that
refuses to apply.

## Authoring rules

1. Put anomaly components on actors in `FullOfficeMap` (or Blueprint children).
   Door lock is an exception: `ADoorInteractable` / `BP_Door` already owns a native
   `DoorLockAnomaly` component, so office doors can lock without extra placement.
   The loop counter is the same: `ALoopNumberSign` already owns a native
   `LoopNumberAnomaly` component.
2. Prefer reusable Actor Components + Interfaces over one-off Blueprint logic.
3. Ensure components register with `UAnomalyManager` on begin play and unregister on end play.
4. For MaterialSwap variants, keep texture/material references intentional; tracked MaterialSwap content is large.
   A variant identical to the slot's normal material is refused at activation, so run
   `AnomalyAuditMaterials` on a loaded floor after authoring: it lists every swap the
   player could not possibly see.
5. Never rely on Sequencer alone to activate or deactivate anomalies.
6. Move anomalies need destinations. Place two or three `AAnomalyMovePoint` actors and
   either list them on the component or match them by `MoveTargetTag`. The legacy
   single `AnomalyLocation` is ignored while it is zero: a world-space zero is the
   world origin, and shipping that default is what teleports objects onto a floor
   that does not exist.
7. Fill `AnomalyZone` and `AnomalyObjectKind` (details panel, **Anomaly > AI Context**).
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
| `AnomalyFlicker` | Force every light-flicker anomaly |
| `AnomalyPhone` | Force every phone / audio anomaly |
| `AnomalyPursuer` | Force the pursuer |
| `AnomalyMove` | Force every Move anomaly |
| `AnomalyDoor` | Force every DoorLock anomaly |
| `AnomalyMaterial` | Force every MaterialSwap anomaly |
| `AnomalyLoopNumber` | Force the loop-counter `?` glitch |
| `AnomalyForce <filter> [matIndex]` | Force matches by type/class/actor; optional MaterialSwap index |
| `AnomalyAuditMaterials` | List material swaps that would be invisible if they fired |
| `AudioStatus` | Report why the floor is silent: audio device, volumes, music bed, placed ambience |
| `AnomalyHelp` | Print usage |

These are **tilde console** commands in PIE / Standalone / Development. They are compiled out of Shipping.

Filter notes:

- Type labels match exactly (case-insensitive), plus short names (`Flicker`, `Audio`, `Pursuer`, `Phone`, `Door`, `DoorLock`, `LoopNumber`, `Counter`).
- Class/actor partial filters require at least 3 characters.
- Examples: `Flicker`, `Phone`, `Pursuer`, `MaterialSwap`, `Move`, `I01`.

## Achievements tied to anomalies

Spotting achievements unlock on correct lit-elevator calls while the matching type is active. Meta achievement `ACH_SPOT_ALL` requires all nine original types across runs (persisted). `LoopNumber` is a tenth elevator-counted type without a Steam spot achievement. Details: [`../STEAM_ACHIEVEMENTS.md`](../STEAM_ACHIEVEMENTS.md).
