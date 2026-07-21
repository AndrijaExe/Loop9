# Localization

Loop 9 ships interface/text localization for:

- English (`en`) — native source
- Serbian (`sr`)
- German (`de`)
- French (`fr`)
- Russian (`ru`)

## Pipeline

Config: [`../Config/Localization/Game.ini`](../Config/Localization/Game.ini)

One GatherText command:

1. gathers `NSLOCTEXT` / `LOCTEXT` from `Source/Loop9`
2. gathers `FText` from assets under `Content/`
3. imports translations from `Content/Localization/Game/<culture>/Game.po`
4. compiles `Game.locres`
5. re-exports updated PO files

Example:

```bash
UnrealEditor-Cmd /path/to/Loop9.uproject -run=GatherText -config="Config/Localization/Game.ini"
```

On Windows, use the matching `UnrealEditor-Cmd.exe` path for your UE 5.8 install.

## Workflow

1. Add/change English source strings in C++ or assets.
2. Run GatherText.
3. Translate empty `msgstr` entries in each culture’s `Game.po`.
4. Run GatherText again to compile locres.
5. Verify in-editor language switching via settings.

## Authoring rules

- Prefer `FText` / `NSLOCTEXT` over raw `FString` for player-facing text.
- Keep Steam achievement display names/descriptions in Steamworks localization separately; in-game achievement unlocks use API names, not localized Steam strings.
- Chat thinking indicators and initial Dragojlo rules are localized in code/PO files.
- Backend moderation fallbacks are localized server-side for EN/SR/DE/FR/RU.

## Validation tips

- Switch language in settings and spot-check main menu, pause, chat, endings, and interaction prompts.
- Watch for UTF-8 BOM issues if external tools rewrite PO files; Unreal is generally tolerant, but `msgfmt` may complain.
- Confirm staged cultures in `DefaultGame.ini` match the five supported languages.
