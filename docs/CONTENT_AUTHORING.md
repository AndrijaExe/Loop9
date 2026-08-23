# Content Authoring

This guide is for map and Blueprint authors working in Unreal Editor 5.8.

## Primary assets

| Asset | Path / role |
|---|---|
| Main map | `Content/MyStuff/Maps/FullOfficeMap.umap` |
| Game Mode BP | `Content/MyStuff/Blueprints/GameMode/BP_Loop9GameMode.uasset` |
| Elevator director BP | `Content/MyStuff/Blueprints/Cinematics/BP_LoopElevatorTransitionDirector.uasset` |
| Ending sequences | `Content/MyStuff/Cinematics/Endings/LS_Ending_*.uasset` |
| Ending widgets | `Content/MyStuff/UI/Endings/WBP_Ending_*.uasset` — keep title + why copy; spawn `BuildRunEventCards()` under it |
| Timeline art | `Content/MyStuff/UI/Timeline/` — call / lift / ending icons + archive star plate |
| Shift archive (C++) | `UShiftArchiveWidget` — add a main-menu button named `Archive`; MCP brief: [`HOME_EDITOR_TIMELINE_AND_ARCHIVE.md`](HOME_EDITOR_TIMELINE_AND_ARCHIVE.md) |
| Legacy elevator sequences | `Content/MyStuff/Cinematics/Elevator/LS_Elevator_*` — **deprecated; remove after Reference Viewer confirms unused** |

## Game Mode wiring

`ALoop9GameMode` exposes:

- `EndingWidgetClasses` — map of `ELoopEndingType` → widget class
- `EndingSequences` — map of `ELoopEndingType` → Level Sequence soft reference
- `GameplayNotificationWidgetClass` — optional toast/notification widget

After C++ changes to these properties, open and resave `BP_Loop9GameMode`.

## Elevator director wiring

Place / configure `BP_LoopElevatorTransitionDirector` in the map:

1. Assign `LitElevatorArrivalPoint` (`ATeleportPoint`).
2. Assign `ArrivalDoorWings` for the lit elevator doors.
3. On each relevant `ALiftButton`, set:
   - `TransitionDirector`
   - `TransitionDoorWings` (source doors that close when that button is pressed)
4. Assign audio:
   - `ButtonPressSound`
   - looping `TravelSound`
   - volume / fade-out values as needed

Do **not** reintroduce elevator Level Sequence overrides. Presentation is C++-driven.

## Teleport points

- Register entry/exit/arrival markers via `ATeleportPoint`.
- Elevator cinematic arrival must use the director’s explicit lit arrival point.
- Avoid relying on a random exit teleport for the cinematic path.

## Doors and interactables

- `ALiftDoorWing` opens/closes and broadcasts movement finished.
- `DoorInteractable` and other `Loop9Interactable` actors handle world interactions.
- Inspectables use `InspectableComponent` / inspection stage actors for the black-room inspection flow.

## Anomaly placement

See [ANOMALIES.md](ANOMALIES.md). Checklist for each new anomaly actor:

1. Correct anomaly component type.
2. Registers with `UAnomalyManager`.
3. Visible baseline on loop 1 when inactive.
4. Distinct enough for a first-time player to notice under office lighting.
5. Covered by localization if it adds text.
6. `AnomalyZone` and `AnomalyObjectKind` filled in English under **Anomaly > AI Context**,
   naming a landmark and a category rather than an asset. Rules and examples:
   [ANOMALIES.md](ANOMALIES.md#ai-context-tagging).

### Move anomaly destinations

Drop `AnomalyMovePoint` actors where a moved object is allowed to appear, then point
the component at them. Two ways, either is fine:

- List them in **Move Anomaly > Move Target Points** on the component.
- Or leave that empty, set **Move Target Tag** on the component, and give each point
  the same `PointTag`. The component then finds them in the level, which is easier
  when several objects share a set of hiding places.

The arrow on the actor is the rotation the object receives, so aim it the way the
object should face. `Weight` biases the draw; `bEnabled` retires a point without
deleting it. A point standing where the object already is gets skipped at runtime,
because a move the player cannot see reads as a bug.

Place at least two per object. With none placed, the component logs a warning and
declines to activate rather than moving the object to the world origin.

## UI authoring

- Keep player-facing strings as `FText` / `NSLOCTEXT` so GatherText can collect them.
- Chat thinking indicator strings are localized in code (`Thinking...`, `Still thinking...`).
- Ending widgets must initialize from `UEndingWidget::InitializeEnding`.
- Replacement ending has a dedicated terminal presentation path.
- The main menu synthesizes its Archive and Help buttons at runtime by cloning the
  Settings button, so a WBP that predates them still shows them. Authoring real
  buttons named `Archive` and `Help` in the WBP takes precedence and is preferred
  once the menu is next touched.
- `HelpWidget` builds a readable C++ layout when no WBP is assigned. To art-direct it,
  make a WBP child and set it on the menu's **Help Widget Class**; bind `VB_Sections`
  and the C++ fills the copy in.

## Safe extension workflow

1. Prefer C++ hooks + Blueprint presentation.
2. Rebuild C++ if signatures change.
3. Resave referencing Blueprints/maps.
4. Test Standalone Game.
5. Run Reference Viewer before deleting assets.
6. Update docs/checklist only if behavior or release status changed.
