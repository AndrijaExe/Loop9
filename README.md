# Loop 9

Horror loop game built in Unreal Engine 5.8.

## Repo size

This repo intentionally excludes large marketplace assets and generated Unreal folders.  
Current tracked project size is approximately **215 MB** (source code + custom
content). The July MaterialSwap texture variants account for roughly 158 MB.

Locally the full project is ~19 GB because of third-party content in `Content/`.

## Requirements

- Unreal Engine **5.8**
- Plugins enabled in the project:
  - ModelContextProtocol (MCP)
  - OnlineSubsystemSteam (Steam auth for the backend chat)

## Setup after clone

1. Clone the repo.
2. Copy config templates:
   ```powershell
   copy Config\DefaultGame.ini.example Config\DefaultGame.ini
   copy Config\DefaultEngine.ini.example Config\DefaultEngine.ini
   ```
3. Edit `Config/DefaultGame.ini` and set the production `APIEndpoint`.
   Steam supplies player identity/session auth; do not package a game token.
4. Install the marketplace assets listed below into `Content/` (same folder names).
5. Open `Loop9.uproject` in UE 5.8 and let it compile.

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

## Steam achievements

`Loop9AchievementsSubsystem` tracks gameplay events (elevator decisions, AI
chats, loops, endings, anomaly spotting) and unlocks achievements through the
Online Subsystem (no-op without Steam). Extra achievements can be unlocked from
Blueprint via `UnlockAchievement(ApiName)`.

The full list of 27 achievements (API names, display names, unlock conditions)
and the step-by-step Steamworks publishing guide live in
[`STEAM_ACHIEVEMENTS.md`](STEAM_ACHIEVEMENTS.md).

Achievements cannot be tested on the Spacewar dev App ID (480) — they require
Loop 9 App ID `4982260`, and Steamworks changes must be published before testing.

## Release preparation

The single source of truth for remaining release work is
[`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md). Elevator and ending cinematics
are documented in [`CINEMATIC_SEQUENCE_PLAN.md`](CINEMATIC_SEQUENCE_PLAN.md);
the Unreal MCP editor handoff is
[`UNREAL_MCP_SEQUENCE_HANDOFF.md`](UNREAL_MCP_SEQUENCE_HANDOFF.md).

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
- `Loop9.uproject`

## Push to GitHub

```powershell
git init
git add .
git status
git commit -m "Initial commit: Loop 9 project"
git remote add origin https://github.com/YOUR_USER/YOUR_REPO.git
git branch -M main
git push -u origin main
```
