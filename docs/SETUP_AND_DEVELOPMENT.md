# Setup and Development

## Requirements

- Unreal Engine **5.8**
- Git
- Windows (Shipping / Steam packaging) or Linux (editor development)
- Steam client for auth / achievement testing with App ID `4982260`
- Plugins enabled in `Loop9.uproject`:
  - `OnlineSubsystemSteam`
  - `ModelContextProtocol` (editor automation; optional for play)

## Clone and first open

1. Clone the game repository.
2. Copy config templates (do not commit local secrets):

Windows (PowerShell):

```powershell
copy Config\DefaultGame.ini.example Config\DefaultGame.ini
copy Config\DefaultEngine.ini.example Config\DefaultEngine.ini
```

Linux / macOS:

```bash
cp Config/DefaultGame.ini.example Config/DefaultGame.ini
cp Config/DefaultEngine.ini.example Config/DefaultEngine.ini
```

3. Edit `Config/DefaultGame.ini`:
   - Set production `APIEndpoint` to the live backend chat URL.
   - Keep Steam session auth as the shipping path. Do **not** package a game token.
4. Edit `Config/DefaultEngine.ini`:
   - Set `SteamDevAppId=4982260` for local Steam testing.
5. Install marketplace content into `Content/` (see table below).
6. Open `Loop9.uproject` in UE 5.8 and let it compile `Loop9Editor`.

## Marketplace content (gitignored)

Tracked custom content lives mainly under `Content/MyStuff/`. Large marketplace packs are gitignored and must be installed locally:

| Folder | Approx. size |
|---|---|
| `Office_Pack_Vol_1` | ~5 GB |
| `Deko_MatrixDemo` | ~2.5 GB |
| `MsvFx_Niagara_Explosion_Pack_01` | ~135 MB |
| `Characters` | ~125 MB |
| `Urban_Nomad` | ~96 MB |
| `Fab` | ~36 MB |
| `DAZ` | ~12 MB |

Without these folders, `FullOfficeMap` will show missing references.

## Repo size notes

- Tracked project size is roughly **215 MB** (source + custom content).
- MaterialSwap texture variants account for a large share of tracked content.
- Full local project size is roughly **19 GB** with marketplace assets.

## Play modes

| Mode | Use for |
|---|---|
| PIE | Fast iteration; not authoritative for Steam auth/achievements |
| Standalone Game | Preferred local QA for elevators, endings, input locks |
| Packaged Development | Closer to Shipping; still may include debug commands |
| Shipping | Release candidate; anomaly debug console commands are compiled out |

## Steam development rules

- Achievements and real Steam auth require App ID `4982260` (Spacewar `480` is insufficient).
- Prefer launching through the Steam client, or place a local `steam_appid.txt` containing `4982260` next to the executable for non-packaged testing.
- Never ship `steam_appid.txt`, API keys, or game tokens in the depot.
- Backend must have `STEAM_WEB_API_KEY`, `STEAM_APP_ID=4982260`, and `AUTH_ALLOW_GAME_TOKEN=false` in production.

## Recommended editor workflow

1. Rebuild C++ after pulling subsystem / director changes.
2. Open and resave `FullOfficeMap` plus `BP_LoopElevatorTransitionDirector` if C++ properties changed.
3. Test elevator transitions and at least one ending in Standalone Game.
4. Use anomaly debug commands in non-Shipping builds (see [ANOMALIES.md](ANOMALIES.md)).
5. Keep release status only in [`../RELEASE_CHECKLIST.md`](../RELEASE_CHECKLIST.md).

## Related docs

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [CONTENT_AUTHORING.md](CONTENT_AUTHORING.md)
- [AI_AND_BACKEND_INTEGRATION.md](AI_AND_BACKEND_INTEGRATION.md)
- Backend setup: [`../../../Backend/Loop9_backend/docs/DEVELOPMENT_AND_TESTING.md`](../../../Backend/Loop9_backend/docs/DEVELOPMENT_AND_TESTING.md)
