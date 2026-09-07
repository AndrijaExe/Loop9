# Polish pass — šta ostaje u editoru

Poslednje ažuriranje: **04.09.2026.**

C++ deo polish prolaza je odrađen. Editorski deo (Move destinacije, materijali,
atenuacija, Help/Archive dugmad, figura u meniju, `IA_Flashlight`) je takođe
odrađen — te sekcije su izbačene iz ovog fajla 25.08.2026. jer su bile gotove;
gde su bile trajno korisne, opis je u [ANOMALIES.md](ANOMALIES.md) i
[AUDIO_ASSIGNMENT_CHECKLIST.md](AUDIO_ASSIGNMENT_CHECKLIST.md). Ispod je samo
ono što je još otvoreno.

Legenda: `[ ]` nije odrađeno · `[~]` radi ali vredi proveriti

---

## 0. Sledeća sesija — urađeno 26.08.2026.

- [x] **Replacement terminal: kucanje + SKIP.** C++ default na
  `/Game/MyStuff/Sound/UI/TypewriterKey` (`SC_SFX`, volume 0.55, pitch jitter),
  dugme vidljivo tokom kucanja, label **SKIP**, prvi klik dovrši sav preostali
  tekst, posle toga **Return to Main Menu**.
- [x] **Credits odvojeni od Help-a.** `CreditsWidget` + dugme na meniju
  (Play → How to Play → Settings → Archive → Credits → Quit). Help više nema
  SOUND CREDITS. Freesound autori iz download istorije su u Credits i
  `STORE_PAGE.md`. Steamworks Legal / About usklađen 26.08.2026.
- [x] **Help dugme → How to Play.** `ApplyLocalizedTexts` piše **HOW TO PLAY**.
- [x] GatherText da novi stringovi uđu u `.locres`. Pokrenuto 26.08.2026. uveče.

---

## 1. Uvoz zvuka i lokalizacija (obavezno)

- [x] Uvezi `Content/MyStuff/Sound/UI/TypewriterKey.wav` kao `SoundWave`
  (`/Game/MyStuff/Sound/UI/TypewriterKey`). Uvezeno 25.08.2026. Preporuka:
  **Sound Class = `SC_SFX`**, ne `SC_Music`.
- [x] Uvezi četiri `Content/MyStuff/Sound/Doors/*.wav`
  (`DoorOpen`, `DoorClose`, `DoorLocked`, `DoorBlocked`). `.uasset` postoje
  od 25.08.2026.
- [x] **Gather Text** pokrenut 25.08.2026. (komanda iz
  [LOCALIZATION.md](LOCALIZATION.md)). `.locres` je u repou.

## 2. Sitnice koje vredi proveriti u igri

- [x] **Intenzitet lampe.** Potvrđeno 26.08.2026.: C++ default 2600 lm, cone
  16°/34° je u redu, ne dirati.
- [~] **Move destinacije.** Sva četiri placementa imaju po jednu destinaciju, pa
  je pomeraj isti u svakoj petlji. Ako želiš varijaciju, dodaj 2–3
  `AnomalyMovePoint` aktera po objektu — vidi [ANOMALIES.md](ANOMALIES.md).
- [x] **`AmbientSound_0` u `FullOfficeMap`.** Obrisan 26.08.2026. (svirao je
  `HorrorAmbience1` i duplirao C++ bed). Muziku sprata i dalje pušta
  `ALoop9GameMode`.
- [x] **Slike u Help ekranu.** Ne radimo za sada (26.08.2026.).

---

## 2b. Raspored dugmadi (28.08.2026.)

Oba zadatka su **WBP layout u editoru**, ne C++. Kôd samo klonira postojeće
slotove, pa preuzima šta god zatekne.

- [x] **Main menu: podići vertikalni stack dugmadi.** VerticalBox pomeran;
  potvrđeno na v1.0.3 (04.09.) i editor 07.09. `SynthesizeButtonBefore`
  kopira layout sa template slota, pa sintetizovano `CREDITS` ostaje
  poravnato.
- [x] **Ending ekrani: `Return to Main Menu` pomereno udesno.** C++
  `AlignToCanvasBottomRight` na `BT_Continue` (svih šest `WBP_Ending_*` plus
  `WBP_ReplacementTerminal`). Anchor donji-desni, padding 64×80, veličina
  ostaje 520×72. Fallback layout (kad WBP binding pukne) i dalje centrira.

---

## 3. QA prolaz posle svega

Potvrđeno na **v1.0.3** (04.09.2026.).

- [x] Odigraj 10 petlji i zabeleži koliko ih je imalo anomaliju. Očekivano ~8/10,
  loop 1 uvek čist.
- [x] U tih 10 petlji: Hide i Material/Text treba da se pojave češće nego ranije,
  Pursuer osetno ređe.
- [x] Nijedna aktivna anomalija ne sme biti nevidljiva. Ako se to opet desi,
  `AnomalyList` u konzoli pokazuje šta je aktivno na tom spratu.
- [x] Background muzika radi **od ulaska u nivo**, bez ulaska u settings menu.
- [x] Posle jedne Pursuer anomalije muzika se **vrati** kad Dragojlo nestane.
- [x] Zvuk treperenja se čuje samo blizu tog svetla, ne kroz celu mapu.
- [x] `F` gasi i pali lampu, i ne radi tokom pauze i inspekcije objekta.
- [x] Zaključana vrata: čuje se `DoorLocked`/`DoorBlocked` **i** vrata se blago
  zatresu (2°, 0.35 s). Otvaranje i zatvaranje imaju različite zvuke.
- [x] Ending ekran: tekst se iskucava (~20 znakova/s) sa zvukom tastera i povremenom
  greškom koju "ispravi". Dugme piše **SKIP** dok kuca, pa se vrati na **Return to
  Main Menu**. Prvi klik na SKIP dopuni tekst, ne izbaci te u meni.
- [x] Prebaci jezik na sr/de/fr/ru i proveri chat ("Razmišlja...", poruka o
  predugačkoj poruci), prompt u liftu i `TASK COMPLETE` na kraju.
