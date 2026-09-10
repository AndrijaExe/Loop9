# Loop 9

Psychological horror loop game built in **Unreal Engine 5.8**.

Steam App ID: **4982260**

## Documentation

Start here:

- [docs/INDEX.md](docs/INDEX.md) — full game documentation map
- Cross-repo index: [`../../DOCUMENTATION.md`](../../DOCUMENTATION.md)
- Backend docs: [`../../Backend/Loop9_backend/ARCHITECTURE.md`](../../Backend/Loop9_backend/ARCHITECTURE.md)

Authoritative release / store / achievements docs:

- [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md)
- [STEAM_ACHIEVEMENTS.md](STEAM_ACHIEVEMENTS.md)
- [Marketing/Steam/STORE_PAGE.md](Marketing/Steam/STORE_PAGE.md)

## Repo size

This repo intentionally excludes large marketplace assets and generated Unreal folders.  
Current tracked project size is approximately **215 MB** (source code + custom
content). The July MaterialSwap texture variants account for roughly 158 MB.

Locally the full project is ~19 GB because of third-party content in `Content/`.

## Requirements

- Unreal Engine **5.8**
- Plugins enabled in the project:
  - OnlineSubsystemSteam (Steam auth for the backend chat)
  - ModelContextProtocol (optional editor automation)

## Setup after clone

1. Clone the repo.
2. The tracked default configs already contain startup/cook maps, Steam App ID
   `4982260`, supported cultures, and the public production backend endpoint.
   Steam supplies player identity/session auth; never add a packaged game token.
3. Install the marketplace assets listed below into `Content/` (same folder names).
4. Open `Loop9.uproject` in UE 5.8 and let it compile.

More detail: [docs/SETUP_AND_DEVELOPMENT.md](docs/SETUP_AND_DEVELOPMENT.md).

## Steam auth

When the game runs through Steam with App ID `4982260`, it
exchanges the local Steam session ticket for a short-lived backend token via
`POST /api/auth/steam` and uses it as `X-Session-Token` on chat requests
(`Loop9BackendAuthSubsystem`). The backend then derives the player identity from
the verified Steam ID. Legacy `GameToken` (`X-Game-Token`) auth is available only
for explicit non-production testing: set `bRequireSteamSession=false` in the
client config and `AUTH_ALLOW_GAME_TOKEN=true` on a non-production backend.

Before shipping, confirm `SteamDevAppId=4982260` in `Config/DefaultEngine.ini`
and configure `STEAM_WEB_API_KEY` / `STEAM_APP_ID=4982260` on the backend.

Integration details: [docs/AI_AND_BACKEND_INTEGRATION.md](docs/AI_AND_BACKEND_INTEGRATION.md).

## Steam achievements

`Loop9AchievementsSubsystem` tracks gameplay events (elevator decisions, AI
chats, loops, endings, anomaly spotting) and unlocks achievements through the
Online Subsystem (no-op without Steam). Extra achievements can be unlocked from
Blueprint via `UnlockAchievement(ApiName)`.

The full list of 28 achievements (API names, display names, unlock conditions)
and the step-by-step Steamworks publishing guide live in
[`STEAM_ACHIEVEMENTS.md`](STEAM_ACHIEVEMENTS.md).

Achievements cannot be tested on the Spacewar dev App ID (480) — they require
Loop 9 App ID `4982260`, and Steamworks changes must be published before testing.

## Release preparation

The single source of truth for remaining release work is
[`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md). Cinematics are documented in
[`docs/CINEMATICS_AND_AUDIO.md`](docs/CINEMATICS_AND_AUDIO.md).

## Required marketplace content (not in repo)

These folders are gitignored. Install them from the Epic Marketplace / Fab into `Content/`:

| Folder | Approx. size |
|--------|-------------|
| `Office_Pack_Vol_1` | ~5 GB |
| `Deko_MatrixDemo` | ~2.5 GB |
| `MsvFx_Niagara_Explosion_Pack_01` | ~135 MB |
| `Characters` | ~125 MB |
| `Urban_Nomad` | ~96 MB |
| `Fab` | ~36 MB |
| `DAZ` | ~12 MB |

Without these assets, `FullOfficeMap` will have missing references.

## What is committed

- `Source/` — C++ gameplay code
- `Content/MyStuff/` — custom maps, blueprints, UI, audio
- `Content/FirstPerson/` — base first-person template assets still used by some blueprints
- `Content/Weapons/` — leftover FirstPerson weapon template assets (safe to remove after Reference Viewer check)
- `Config/*.example` and non-sensitive config files
- `docs/` — technical documentation
- `Loop9.uproject`
