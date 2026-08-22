# Unreal MCP Handoff: Session Timeline + Shift Archive

Copy-paste brief for an AI agent with Unreal Engine MCP access.
C++ is already done. Edit existing UMG assets. Not a launch blocker.
Status: [`../RELEASE_CHECKLIST.md`](../RELEASE_CHECKLIST.md) §9.

## Ostalo (za čoveka)

C++ od 21.08.2026. sam crta timeline na ending ekranu i, ako fali, dodaje
`Archive` dugme na main meni. Ne treba Event Graph.

1. Rebuild `Loop9Editor` (hot reload posle `EndingWidget` / `MainMenuWidget`).
2. Potvrdi da editor uveze `Content/MyStuff/UI/Timeline/ui_icon_*.png`.
3. PIE: ending ekran pokazuje title + 2–3 rečenice, ispod kartice, Continue radi.
4. PIE: main menu ima ARCHIVE između Settings i Quit; otvara dosije.
5. Opciono: `ui_archive_star_plate` kao pozadina arhive.
6. Animaciju čvorova **ne radi**.

---

## Objective

1. Show the post-run session timeline on every live ending screen.
2. Add an **Archive** button on the main menu so the existing C++ dossier opens.
3. Import the approved art and use it: three timeline icons + one archive plate.
4. Do not invent new copy. Do not animate nodes.

## Hard constraints

1. Inspect Unreal MCP tool schemas before editing.
2. Pull latest `main` and rebuild `Loop9Editor` first (`FRunEventCard`, `BuildRunEventCards`, `UShiftArchiveWidget`).
3. Do **not** change C++, backend, quotas, Steam Cloud, or `Game.ini` paths.
4. Do **not** replace the ending **title** or the **2–3 why sentences**. Timeline goes **under** them.
5. Do **not** write card titles/bodies in the designer. Use `FRunEventCard.Title` and `.Body`.
6. Do **not** bake ending names or `SHIFT` into the star-plate image. Labels stay widgets.
7. Do **not** add UMG/Sequencer node animations.
8. Do not create `*_v2` ending widgets. Edit the six existing `WBP_Ending_*`.
9. If a required widget name is missing, find the real asset. Do not assume `WBP_MainMenu` exists on disk.

## Approved art (already in the repo)

Folder: `/Game/MyStuff/UI/Timeline/`  
Disk: `Content/MyStuff/UI/Timeline/`

| File | Use |
|---|---|
| `ui_icon_call.png` | Timeline circle for `ERunEventType::Call` |
| `ui_icon_lift.png` | Timeline circle for `CorrectLift` and `WrongLift` |
| `ui_icon_ending.png` | Timeline circle for `Ending` |
| `ui_archive_star_plate.png` | Archive background only. Empty hub + 6 sockets. No text. |

These are source PNGs. After the editor opens they may auto-import as textures. If not, Import them in place.

Texture settings for all four:

- Texture Group: **UI**
- sRGB: on
- Never Stream: on
- Do not use as a world/material texture

## Preflight

1. Pull `main`.
2. Open `Loop9.uproject` in UE 5.8. Compile `Loop9Editor`.
3. Confirm these compile and appear in Blueprints:
   - `FRunEventCard` (`Type`, `Tone`, `LoopIndex`, `Count`, `Title`, `Body`, `RingColor`)
   - `URelationshipSubsystem::BuildRunEventCards()`
   - `UShiftArchiveWidget`
   - `UMainMenuWidget::OnArchiveClicked`
4. Import / reimport the four Timeline textures with the settings above.
5. Locate the live main-menu widget:
   - inspect `MainMenuGameMode` → `MainMenuWidgetClass`
   - find the WBP that already has buttons named `Play`, `Settings`, `Quit`
   - that is the file to edit. It may not be named `WBP_MainMenu`.
6. Open every `/Game/MyStuff/UI/Endings/WBP_Ending_*` and record which text blocks are the title and the why-blurb. Do not delete them.

Live ending widgets:

- `/Game/MyStuff/UI/Endings/WBP_Ending_EscapeTogether`
- `/Game/MyStuff/UI/Endings/WBP_Ending_ObedientFool`
- `/Game/MyStuff/UI/Endings/WBP_Ending_ColdBetrayal`
- `/Game/MyStuff/UI/Endings/WBP_Ending_ParanoidSurvivor`
- `/Game/MyStuff/UI/Endings/WBP_Ending_MergedMemory`
- `/Game/MyStuff/UI/Endings/WBP_Ending_TheReplacement`

Parent class is `UEndingWidget`. It fires `BP_OnEndingInitialized`.

## Task 1 — Timeline on each ending WBP

On each `WBP_Ending_*` (or one shared parent used by all six):

1. Keep title + why text exactly as they are.
2. Remove or hide any `Resets | AI interactions` stats line if present.
3. Add a vertical box **below** the why text. Suggested name: `VB_Timeline`.
4. On `BP_OnEndingInitialized` (or after `InitializeEnding`):

```text
Game Instance
  → Get Subsystem (RelationshipSubsystem)
  → Build Run Event Cards
  → For Each FRunEventCard: add one row to VB_Timeline
```

5. Each row, left to right:
   - thin vertical rail (ice blue)
   - circle
   - card with `Title` (bold) and `Body` (smaller)
6. Circle **outline** = `RingColor` from the card. C++ already sets it:

| `Type` / `Tone` | Ring |
|---|---|
| `Call` Neutral | ice blue `(0.5, 0.8, 1.0)` |
| `Call` Friendly | green |
| `Call` Hostile | red |
| `Call` Suspicious | yellow |
| `CorrectLift` / `WrongLift` / `Ending` | ice blue |

7. Circle **fill / brush** = the matching icon:
   - `Call` → `ui_icon_call`
   - `CorrectLift` or `WrongLift` → `ui_icon_lift`
   - `Ending` → `ui_icon_ending`
8. Prefer a small reusable `WBP_RunEventRow` with `Title`, `Body`, icon, ring color. If you create it, keep it under `/Game/MyStuff/UI/Timeline/`.
9. Two calls on one floor are already collapsed in C++ (`Count`, title `TWO CALLS`). One row is correct.

Do not graph raw Trust / Kindness / Suspicion numbers.

## Task 2 — Archive button on the main menu

1. Open the live main-menu WBP (from Preflight step 5).
2. Duplicate the existing `Settings` (or `Play`) button widget.
3. Name the new widget exactly **`Archive`**. The name must match. C++ does `GetWidgetFromName("Archive")`.
4. Place it in the same column as Play / Settings / Quit, same style.
5. Do not bind OnClicked in Blueprint. `UMainMenuWidget` already binds it and sets the label to localized `ARCHIVE`.
6. Click opens `UShiftArchiveWidget` unless `ArchiveWidgetClass` is set on the menu BP.

Without a widget named `Archive`, the dossier cannot open.

## Task 3 — Optional archive plate (only if Task 2 works)

Art: `ui_archive_star_plate`.

1. You may create `/Game/MyStuff/UI/MainMenu/WBP_ShiftArchive` as a child of `UShiftArchiveWidget`.
2. If you do, keep bind names C++ already looks for: `VB_Nodes`, `TB_Title`, `BT_Back`.
3. Put the star plate as a full-bleed / centered **background Image**. Do not draw names on the texture.
4. Overlay 7 text slots on the empty nodes:
   - center: `SHIFT` (C++ already has `ArchiveHub`)
   - six endings around it, same order as `Loop9RuntimePolicies::AllEndingTypes()`:
     EscapeTogether, ObedientFool, ColdBetrayal, ParanoidSurvivor, MergedMemory, TheReplacement
5. Unlocked = ice-blue name. Locked = `???` + gray. Data comes from `SeenEndings` via C++.
6. If overlaying 7 labels on the plate is messy, leave the C++ vertical list (`VB_Nodes`) and only use the plate as atmosphere behind it. That is acceptable.

## Localization

Timeline card strings are already `LOCTEXT` (`Loop9RunEvent`) with PO translations in
`Content/Localization/Game/<culture>/Game.po` (en/sr/de/fr/ru). Ending titles reuse `Loop9Endings`.

After widget edits, compile locres:

```text
UnrealEditor-Cmd <Loop9.uproject> -run=GatherText -config="Config/Localization/Game.ini"
```

New widget keys to expect: `ARCHIVE`, `SHIFT ARCHIVE`, `BACK`, `???`.
Without locres the game still shows the English C++ source.

## QA

- [ ] Rebuild succeeded; `BuildRunEventCards` is callable from the ending WBP
- [ ] Each of the 6 ending screens still shows its own title + why text
- [ ] Timeline appears under that text, not instead of it
- [ ] Two chats on one floor = one `TWO CALLS` row
- [ ] Insult / doubt / warmth = red / yellow / green ring; icons match Call / Lift / Ending
- [ ] Main menu has `Archive`; click opens the dossier
- [ ] One unlocked ending is blue with its name; the others are `???` and gray
- [ ] Floor with no call still drops Dependency (existing C++; do not reimplement)
- [ ] If any Blueprint still calls `RegisterPlayerMessage`, delete that node
- [ ] GatherText run; smoke EN and SR on ending + archive
- [ ] No new node animation

## Do not touch

- Backend `[STATE]KINDNESS;SUSPICION;DEPENDENCY`
- Daily / monthly quotas
- Steam Cloud Auto-Cloud path (`Game.ini`)
- Ending Level Sequences (separate brief: [`UNREAL_MCP_ENDING_SCENES_HANDOFF.md`](UNREAL_MCP_ENDING_SCENES_HANDOFF.md))
