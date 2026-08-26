# Polish pass — šta ostaje u editoru

Poslednje ažuriranje: **26.08.2026.**

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
  `STORE_PAGE.md`. Steamworks Legal / About treba da se uskladi i Publish-uje.
- [x] **Help dugme → How to Play.** `ApplyLocalizedTexts` piše **HOW TO PLAY**.
- [ ] GatherText da novi stringovi uđu u `.locres`. PO je već popunjen.

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

- [ ] **Intenzitet lampe.** C++ default je 2600 lm, cone 16°/34°, blago topla
  boja. Namerno ne briše mrak; ako je pretamno digni `Intensity` na komponenti
  `Flashlight` u BP-u karaktera.
- [~] **Move destinacije.** Sva četiri placementa imaju po jednu destinaciju, pa
  je pomeraj isti u svakoj petlji. Ako želiš varijaciju, dodaj 2–3
  `AnomalyMovePoint` aktera po objektu — vidi [ANOMALIES.md](ANOMALIES.md).
- [ ] **`AmbientSound_0` u `FullOfficeMap`.** Muziku na spratu sada pušta
  `ALoop9GameMode` kao 2D zvuk, a ovaj akter se pri startu utiša namerno da se
  `HorrorAmbience1` ne bi svirao dva puta. Kod ga gasi sam, ali mapa je jasnija
  bez njega. Drugi `AmbientSound` akteri koji **nisu** na `SC_Music` se ne diraju.
- [ ] **Slike u Help ekranu (opciono).** Napravi WBP dete od `HelpWidget`, bindaj
  `VB_Sections` i postavi ga na **UI > Help Widget Class** na meniju. C++ i dalje
  ubacuje tekst, ti dodaješ vizual oko njega.

---

## 3. QA prolaz posle svega

- [ ] Odigraj 10 petlji i zabeleži koliko ih je imalo anomaliju. Očekivano ~8/10,
  loop 1 uvek čist.
- [ ] U tih 10 petlji: Hide i Material/Text treba da se pojave češće nego ranije,
  Pursuer osetno ređe.
- [ ] Nijedna aktivna anomalija ne sme biti nevidljiva. Ako se to opet desi,
  `AnomalyList` u konzoli pokazuje šta je aktivno na tom spratu.
- [ ] Background muzika radi **od ulaska u nivo**, bez ulaska u settings menu.
  Ako ne radi, otvori konzolu i pokreni **`AudioStatus`** — ispisaće da li postoji
  audio device, koje su jačine, da li muzički bed svira i šta je sa svakim
  `AmbientSound` akterom u mapi. Pošalji mi taj ispis, to je dovoljno da se vidi
  gde je stalo.
- [ ] Posle jedne Pursuer anomalije muzika se **vrati** kad Dragojlo nestane. Ovo
  je bio bug: brojač utišavanja nije padao na nulu kad se komponenta uništi, a
  reset petlje ne učitava novi svet, pa je sprat ostajao tih do kraja sesije.
- [ ] Zvuk treperenja se čuje samo blizu tog svetla, ne kroz celu mapu.
- [ ] `F` gasi i pali lampu, i ne radi tokom pauze i inspekcije objekta.
- [ ] Zaključana vrata: čuje se `DoorLocked`/`DoorBlocked` **i** vrata se blago
  zatresu (2°, 0.35 s). Otvaranje i zatvaranje imaju različite zvuke.
- [ ] Ending ekran: tekst se iskucava (~20 znakova/s) sa zvukom tastera i povremenom
  greškom koju "ispravi". Dugme piše **SKIP** dok kuca, pa se vrati na **Return to
  Main Menu**. Prvi klik na SKIP dopuni tekst, ne izbaci te u meni.
- [ ] Prebaci jezik na sr/de/fr/ru i proveri chat ("Razmišlja...", poruka o
  predugačkoj poruci), prompt u liftu i `TASK COMPLETE` na kraju — to su stringovi
  prevedeni 25.08.2026.
