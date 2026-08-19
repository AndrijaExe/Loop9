# Kod kuće: session timeline + shift archive

C++ za ova dva feature-a je urađen. Ovo je samo Unreal Editor / art / lokalizacija.
Nije launch bloker. Status u [`../RELEASE_CHECKLIST.md`](../RELEASE_CHECKLIST.md) §9.

Rebuilduj `Loop9Editor` pre svega (novi `FRunEvent`, `ERunEventTone`, `UShiftArchiveWidget`).

---

## Šta je već u kodu

### Session timeline (posle smene)

- `RelationshipSubsystem::RunEvents` — log jednog runa
- Tipovi: `Call`, `CorrectLift`, `WrongLift`, `Ending`
- Ton poziva iz AI `[STATE]`: Hostile / Suspicious / Friendly / Neutral
- Briše se na novi run; nije na Steam Cloud

### Shift archive (main menu)

- `UShiftArchiveWidget` — dosije: čvor `SHIFT` + 6 krajeva
- Otključano čita `SeenEndings` iz `Game.ini` (Cloud već syncuje taj fajl)
- Zaključano: `???` + siva šina
- Main menu traži widget imena **`Archive`**

---

## 1. Session timeline na ending ekranu

Živi ekrani su `Content/MyStuff/UI/Endings/WBP_Ending_*`, ne C++ fallback.
Bez izmene tih WBP-ova igrač i dalje vidi samo naslov + kratak tekst.

Na svakom `WBP_Ending_*` (ili na zajedničkom parentu):

1. Ostavi **naslov** i **2–3 rečenice zašto taj kraj**.
2. Umesto linije `Resets | AI interactions` stavi vertikalni timeline.
3. Čitaj `GetGameInstance()` → `RelationshipSubsystem` → `RunEvents`.
4. Svaki event: plava šina + krug (sličica unutra) + kartica desno.

Boja **rubova kruga** samo za `Call`:

| `Tone` | Rub |
|---|---|
| Neutral | plava `#7EC8FF` / `(0.5, 0.8, 1.0)` |
| Friendly | zelena |
| Hostile | crvena |
| Suspicious | žuta |

Lift i ending ostaju plavi. Dva poziva u istom krugu su već spojena (`Count`).

Sličice: 64px isečci (telefon, lift, pečat). Nije potreban novi art pass.

---

## 2. Archive dugme na main menu

Asset: main menu WBP (isti gde su `Play`, `Settings`, `Quit`).

1. Dodaj dugme tačno imena **`Archive`**.
2. C++ ga sam binduje i stavlja label `ARCHIVE`.
3. Klik otvara `UShiftArchiveWidget` (ili `ArchiveWidgetClass` ako ga staviš na menu BP).

Bez ovog imena dosije se ne može otvoriti.

---

## 3. Opcioni Unreal polish (kasnije)

- Pravi star-graph: `SHIFT` u sredini, 6 čvorova, plave/sive linije
- Ikonice na krugovima umesto pune plave tačke
- Poseban `WBP_ShiftArchive` child ako hoćeš layout u designeru; ostavi `VB_Nodes` i `BT_Back` ako želiš da C++ i dalje puni listu
- Session timeline animacija (čvorovi redom)

---

## 4. Lokalizacija

Posle novih stringova u widgetima:

```text
UnrealEditor-Cmd <Loop9.uproject> -run=GatherText -config="Config/Localization/Game.ini"
```

Zatim prevodi u `de/fr/ru/sr` PO i smoke na 5 jezika. Vidi [`LOCALIZATION.md`](LOCALIZATION.md).

Novi ključevi uključuju `ARCHIVE`, `SHIFT ARCHIVE`, `BACK`, `???`.

---

## 5. Brzi QA u editoru

- [ ] Dva chata u istom krugu, treći = signal-drop
- [ ] Uvreda / predaja odluke / sumnja → crveni / dependency / žuti ton na logu (kad timeline bude na WBP)
- [ ] Sprat bez poziva → Dependency padne
- [ ] Završi jedan kraj → Archive pokazuje to ime plavo, ostalo `???`
- [ ] Drugi PC / obrisan local `Game.ini` + Steam Cloud → isti otključani krajevi
- [ ] Ako Blueprint još zove `RegisterPlayerMessage`, obriši taj čvor

---

## Šta ne dirati ovde

- Backend `[STATE]KINDNESS;SUSPICION;DEPENDENCY` — već na Renderu posle deploy-a
- Daily/monthly kvote
- Steam Cloud Auto-Cloud putanja (`Game.ini`)
