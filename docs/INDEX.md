# Loop 9 Game Documentation

English technical documentation for the Unreal Engine 5.8 client.

Cross-repo index (sibling folder): [`../../../DOCUMENTATION.md`](../../../DOCUMENTATION.md)

## Documents

| Document | Purpose |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Subsystems, actors, ownership, game↔backend boundary |
| [SETUP_AND_DEVELOPMENT.md](SETUP_AND_DEVELOPMENT.md) | Clone, UE 5.8, configs, marketplace, Steam Dev App ID |
| [CURSOR_HOME_WORKER.md](CURSOR_HOME_WORKER.md) | Windows home PC as a Cursor My Machines worker (Unreal + MCP) |
| [GAMEPLAY_SYSTEMS.md](GAMEPLAY_SYSTEMS.md) | Loops, elevators, relationships, endings, achievements |
| [ROADMAP_1_1.md](ROADMAP_1_1.md) | `develop` branch scope: Dragojlo memory, ringing phones, new anomalies, secret ending |
| [LORE.md](LORE.md) | Story proposal: the nine, why anomalies exist, how the six endings read |
| [ANOMALIES.md](ANOMALIES.md) | Nine anomaly types, manager lifecycle, debug commands |
| [CONTENT_AUTHORING.md](CONTENT_AUTHORING.md) | Map/Blueprint wiring and safe extension workflows |
| [CINEMATICS_AND_AUDIO.md](CINEMATICS_AND_AUDIO.md) | Elevator C++ transition, ending sequences, audio hooks |
| [UNREAL_MCP_ENDING_SCENES_HANDOFF.md](UNREAL_MCP_ENDING_SCENES_HANDOFF.md) | Exact MCP task brief for polishing all six ending mini-scenes |
| [UNREAL_MCP_THE_EXIT_HANDOFF.md](UNREAL_MCP_THE_EXIT_HANDOFF.md) | MCP task brief for the 1.1 `LS_TheExit` cutscene (camera/lights/audio; geometry is hand work) |
| [HOME_EDITOR_TIMELINE_AND_ARCHIVE.md](HOME_EDITOR_TIMELINE_AND_ARCHIVE.md) | MCP brief: ending timeline, Archive button, Timeline art |
| [AI_AND_BACKEND_INTEGRATION.md](AI_AND_BACKEND_INTEGRATION.md) | Auth, chat, telemetry, timeouts, thinking UI |
| [LOCALIZATION.md](LOCALIZATION.md) | Five-language GatherText / PO / locres workflow |
| [QA_PLAYBOOK.md](QA_PLAYBOOK.md) | Regression matrices, Steam/AI tests, Shipping smoke |

## Authoritative sibling docs (repo root)

| Document | Purpose |
|---|---|
| [`../README.md`](../README.md) | Short onboarding |
| [`../RELEASE_CHECKLIST.md`](../RELEASE_CHECKLIST.md) | Sole release tracker |
| [`../STEAM_ACHIEVEMENTS.md`](../STEAM_ACHIEVEMENTS.md) | Achievement API names and Steamworks setup |
| [`../Marketing/Steam/STORE_PAGE.md`](../Marketing/Steam/STORE_PAGE.md) | Store copy |

## Source of truth rules

1. **Code wins** over prose. Prefer `Source/Loop9/**/*.h` and subsystem implementations.
2. Do not duplicate release checklists, achievement tables, or store marketing here.
3. Elevator presentation is owned by `ALoopElevatorTransitionDirector` (actor), not a Level Sequence and not a WorldSubsystem.
4. Backend contracts are documented in the sibling backend repo; this folder only documents the client side.
