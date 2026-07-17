# Loop 9

Horror loop game built in Unreal Engine 5.8.

## Repo size

This repo intentionally excludes large marketplace assets and generated Unreal folders.  
Expected GitHub size: **~50 MB** (source code + custom content).

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
3. Edit `Config/DefaultGame.ini` and set your `APIEndpoint`, `PlayerId`, and `GameToken`.
4. Install the marketplace assets listed below into `Content/` (same folder names).
5. Open `Loop9.uproject` in UE 5.8 and let it compile.

## Steam auth

When the game runs through Steam (or with `SteamDevAppId=480` for testing), it
exchanges the local Steam session ticket for a short-lived backend token via
`POST /api/auth/steam` and uses it as `X-Session-Token` on chat requests
(`Loop9BackendAuthSubsystem`). The backend then derives the player identity from
the verified Steam ID. Legacy `GameToken` (`X-Game-Token`) auth is available only
for explicit non-production testing: set `bRequireSteamSession=false` in the
client config and `AUTH_ALLOW_GAME_TOKEN=true` on a non-production backend.

Before shipping, replace `SteamDevAppId` in `Config/DefaultEngine.ini` with your
real App ID and configure `STEAM_WEB_API_KEY` / `STEAM_APP_ID` on the backend.

## Steam achievements

`Loop9AchievementsSubsystem` tracks gameplay events (elevator decisions, AI
chats, loops, endings, anomaly spotting) and unlocks achievements through the
Online Subsystem (no-op without Steam). Extra achievements can be unlocked from
Blueprint via `UnlockAchievement(ApiName)`.

The full list of 25 achievements (API names, display names, unlock conditions)
and the step-by-step Steamworks publishing guide live in
[`STEAM_ACHIEVEMENTS.md`](STEAM_ACHIEVEMENTS.md).

Achievements cannot be tested on the Spacewar dev App ID (480) — they require
your real App ID, and Steamworks changes must be published before testing.

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
