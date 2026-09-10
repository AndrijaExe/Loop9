# Anomalies

Loop 9 has **twelve** anomaly types (`ELoopAnomalyType` in `Anomaly/AnomalyTypes.h`): ten at v1.0.6, plus `Watcher` and `Creep` in 1.1. There is no Clock anomaly. `ACH_SPOT_ALL` tracks all twelve (`ACH_SPOT_LOOPNUMBER`, `ACH_SPOT_WATCHER`, `ACH_SPOT_CREEP` included).

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
| Watcher (1.1) | `WatcherAnomaly` | `UWatcherAnomalyComponent`: a figure standing with its back turned |
| Creep (1.1) | `CreepAnomaly` | `UCreepAnomalyComponent`: an object that drifts ~1 cm/s while the player is on the floor |

`MaterialSwapAnomalyComponent` currently reports type `Text` and is used for material/text visual variants.

**Ringing phones (1.1).** An `AudioAnomalyComponent` placed on an `AAI_Friend`
desk phone with `bAnswerable` (default) is a ringing phone: interacting while
it rings stops the sound (`Answer()`, anomaly stays active), opens the chat
with one local canned line (`ChatRingingPhoneAnswered`, no backend call, no
message slot), logs `object_inspected` / `ringing_phone`, and cuts the line for
the whole floor (`UAnomalyManager::CutPhoneLineForFloor`). Every phone then
answers `SayToAI` with `ChatLineCutAfterRing` until the next floor. Audio
components on any other actor behave as before.

**Watcher (1.1).** `UWatcherAnomalyComponent` sits on an empty anchor actor and
spawns `FigureClass` there, rotated so its back faces the player. It never
moves. The manifestation vanishes when the player comes within
`VanishDistance`, on the second look after looking away, after
`MaxContinuousLookSeconds`, or after `MaxLifetimeSeconds`; the component stays
active so the floor still judges "lit". First sight logs `object_inspected` /
`figure_back_turned`. Selection weight 0.45 (rare, like the Pursuer). Spot
achievement `ACH_SPOT_WATCHER` (hidden).

**Contact (1.1).** Reaching `VanishDistance` while closing in faster than
`ContactApproachSpeed` (420 cm/s, i.e. sprinting at him) is a collision, not a
look: `UWatcherAnomalyComponent::Contact` plays a full-screen "bad signal"
burst (`ALoop9PlayerController::PlaySignalBurst`, `USignalBurstWidget`, built in
code, no asset), cuts every light on the floor for `ContactBlackoutSeconds`
(5 s; 0 = until the next floor) through `ULoop9LightsSubsystem`, unlocks
`ACH_TOO_CLOSE`, and only then removes the figure. A slow approach still just
makes him vanish.

**Ringing floor (1.1).** A ringing desk phone is a whole beat, run by
`ULoop9RingingFloorSubsystem` (world subsystem, nothing to place). When the
player steps out of the lit lift (`LiftExitDistanceCm` from the arrival point),
its doors close and the lit button refuses presses; every light goes out except
the lamp nearest the phone (`ULoop9LightsSubsystem::FindNearestLight`, within
`PhoneLampSearchRadiusCm`). Picking up shows one of six lines
(`AAI_Friend::PickRingingPhoneLine`, `ChatRingingPhoneAnswered`,
`ChatRingingLine2..6`) read-only (`UAI_ChatWidget::SetInputLocked`); lines 4-6
hint at the wall, the stairs and the street door of the secret ending. Closing
the chat restores the lights and reopens the lit lift. The dark lift is never
held. `MaxHoldSeconds` (240) reopens the lift if nobody ever answers; a loop
change restores everything. Achievement `ACH_WRONG_NUMBER` on pickup.

**Text anomaly textures (1.1 rework).** The `MaterialSwap` variants under
`Content/MyStuff/Anomalies/{I01,Magazine,D01,F01}` are generated, not painted:
`py -3 Tools/make_text_anomaly_textures.py <folder>` rebuilds every `_C`/`_C2`/`_C3`
(and the Magazine `_E` emissives) from the clean Deko base textures, so the words
read as print rather than as a sticker: a centred top-band headline and a swapped
headline column on the newspaper (I01), a "next issue" teaser in the cover's own
condensed type on the PC magazine back cover (Magazine), a CRT prompt on the
manual's monitor (D01), and the book's own green title block plus spine tag (F01).
Round trip without opening the editor:

```
UnrealEditor-Cmd.exe Loop9.uproject -EnablePlugins=PythonScriptPlugin -ExecutePythonScript=Tools/EditorPython/export_anomaly_textures.py -unattended -nopause -nosplash
py -3 Tools/make_text_anomaly_textures.py D:\Temp\anomaly_textures
UnrealEditor-Cmd.exe Loop9.uproject -EnablePlugins=PythonScriptPlugin -ExecutePythonScript=Tools/EditorPython/reimport_anomaly_textures.py -unattended -nopause -nosplash
```

The reimport writes over the existing assets, so the material instances keep their
references. Run `AnomalyAuditMaterials` afterwards; a variant identical to the
baseline is refused at activation.

**Creep (1.1).** `UCreepAnomalyComponent` goes on the object itself. On
activation the object starts at its normal spot and drifts toward a target at
`CreepSpeedCmPerSecond` (1.0): a tagged `AAnomalyMovePoint` (`CreepTargetTag`,
same contract as Move) or `CreepOffset` in the object's own axes (default 60 cm
sideways). It stops when it arrives and never comes back on its own; a
destination under 5 cm away is refused. `bPauseWhileObserved` (off) makes it
move only when the player is not looking. Type weight 1.0. Spot achievement
`ACH_SPOT_CREEP`. Debug: `AnomalyCreep`, filters `Creep` / `Drift` / `SlowMove`.

Pursuer is the one anomaly that changes the phone. While it is active
(`UAnomalyManager::IsAnomalyTypeActive(Pursuer)` — true for the whole floor
visit, even after the manifestation despawns), `AAI_Friend::SayToAI` never
reaches the backend: the chat shows the localized `ChatPursuerNoAnswer` line
(nobody on the line, the breathing is in the room), no message slot is spent,
no relationship delta or AI interaction is recorded. The backend additionally
refuses to plant a location or a wrong lift on a Pursuer floor, so a stale
client cannot get one either.

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
7. **Scale** is implemented (`UScaleAnomalyComponent`) but was never placed on an
   actor in `FullOfficeMap` — the umap only has the class in its name table.
   At world begin play, if no Scale component registered, `UAnomalyManager`
   attaches one to `SM_ComputerPrinter_A01_N1` (Zoran / "ništa u mašini"),
   1.4×, zone `the back shelves past the lifts`. Prefer adding the component in the editor so
   this fallback is not needed; the runtime attach skips when a map placement
   exists.
8. Fill `AnomalyZone` and `AnomalyObjectKind` (details panel, **Anomaly > AI Context**).
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

### Observation zone volumes

Location misdirection is measurable only for authored map zones. Place one
`ALoop9ObservationZoneVolume` per coarse `AnomalyZone` area, not per anomaly
object.
`ZoneId` is the normalized label (`the north corridor` → `north_corridor`).
The volume is a passive overlap sensor registered with
`ULoop9ObservationJournalSubsystem`: it never changes anomaly state and never
calls `ULoopManagerSubsystem` directly.

Only inactive authored zones with a matching volume can become `decoy_zone`;
the zone the player is currently standing in is also excluded. This prevents
the AI from naming a nonexistent, unmeasurable, or already-checked place. The
registry query avoids scanning world actors.
Entering the zone counts only after a validated `misdirect_location` response;
entering it earlier has no effect. Missing volumes safely disable location
deception for that zone and leave truthful guidance available.

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
| `AnomalyHide` | Force every Hide anomaly |
| `AnomalyMove` | Force every Move anomaly |
| `AnomalyDoor` | Force every DoorLock anomaly |
| `AnomalyMaterial` | Force every MaterialSwap anomaly |
| `AnomalyText` | Force every Text anomaly (swaps + spawned notes) |
| `AnomalyScale` | Force every Scale anomaly |
| `AnomalyPhantom` | Force every PhantomMessage anomaly |
| `AnomalyLoopNumber` | Force the loop-counter `?` glitch |
| `AnomalyWatcher` | 1.1: force the back-turned Watcher figure |
| `AnomalyForce <filter> [matIndex]` | Force matches by type/class/actor; optional MaterialSwap index |
| `AnomalyAuditMaterials` | List material swaps that would be invisible if they fired |
| `PhoneLineRestore` | 1.1: undo the floor-wide line cut after answering a ringing phone (re-test without changing floors) |
| `DragojloMemory` | 1.1: print the persisted cross-run memory (`runs`, `last`, `tone`, `lies`, ...) |
| `DragojloForget` | 1.1: wipe that memory, as on a fresh install |
| `AudioStatus` | Report why the floor is silent: audio device, volumes, music bed, placed ambience |
| `AnomalyHelp` | Print usage |
| `EndingSetup TheExit` | 1.1: arm the ground-floor door (see `EndingHelp`) |

These are **tilde console** commands in PIE / Standalone / Development. They are compiled out of Shipping.

Filter notes:

- Type labels match exactly (case-insensitive), plus short names (`Hide`, `Flicker`, `Audio`, `Pursuer`, `Phone`, `Door`, `DoorLock`, `Scale`, `Phantom`, `LoopNumber`, `Counter`, `Watcher`, `Figure`, `BackTurned`).
- Class/actor partial filters require at least 3 characters.
- Examples: `Flicker`, `Phone`, `Pursuer`, `MaterialSwap`, `Move`, `I01`.

## Achievements tied to anomalies

Spotting achievements unlock on correct lit-elevator calls while the matching type is active. Meta achievement `ACH_SPOT_ALL` requires all twelve types across runs (persisted): the original nine, `LoopNumber` (v1.0.6), `Watcher` and `Creep` (1.1). Two event achievements are not tied to the lift: `ACH_WRONG_NUMBER` (answered the ringing phone) and `ACH_TOO_CLOSE` (ran into the Watcher). Details: [`../STEAM_ACHIEVEMENTS.md`](../STEAM_ACHIEVEMENTS.md).
