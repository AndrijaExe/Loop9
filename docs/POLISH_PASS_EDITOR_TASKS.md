# Polish pass — šta ostaje u editoru

Poslednje ažuriranje: **25.08.2026.**

C++ deo polish prolaza je odrađen i kompajlira se sam. Ono što je trebalo
u editoru (Move, materijali, atenuacija, Help/Archive dugmad, figura u meniju,
`IA_Flashlight`) je odrađeno. Ostaju dva uvoza zvuka, Compile Text i QA u igri.

Legenda: `[ ]` nije odrađeno · `[~]` radi ali vredi proveriti · `[x]` gotovo

---

## 0. Uvoz zvuka i lokalizacija (obavezno, novo 25.08.2026.)

- [ ] Uvezi `Content/MyStuff/Sound/UI/TypewriterKey.wav` kao `SoundWave`
  (`/Game/MyStuff/Sound/UI/TypewriterKey`). To je kucanje na endinzima. Bez uvoza
  tekst se i dalje iskucava, samo nema zvuka — `EndingWidget` je otporan na
  nedostajući asset. Preporuka: **Sound Class = `SC_SFX`**, ne `SC_Music`, da ne
  ide kroz ambient slider.
- [ ] Uvezi četiri `Content/MyStuff/Sound/Doors/*.wav` ako još nisu uvezeni
  (`DoorOpen`, `DoorClose`, `DoorLocked`, `DoorBlocked`). U repo-u su samo `.wav`
  fajlovi, bez `.uasset`, pa proveri da li `ADoorInteractable` nalazi zvuke.
- [ ] Pokreni **Gather Text** pa **Compile Text**. Bez toga izmene Help teksta i
  novi `SKIP` label rade samo na engleskom, jer igra čita `.locres`, ne `.po`.

## 1. Move anomalije — spawn pointovi (obavezno)

Bez ovoga Move anomalije **neće da se aktiviraju** — namerno. Umesto da objekat
odu na koordinatu `(0,0,0)`, komponenta upiše warning u log i propusti se, pa
manager izvuče drugu anomaliju. To je ono što je uzrokovalo Dragojla na
nepostojećem trećem spratu.

Četiri placementa u `FullOfficeMap` (24.08.2026), svaki na **legacy
`AnomalyLocation`** (nema `AnomalyMovePoint` aktera). Sve destinacije su >20 cm
od rest poze, pa se Move aktivira. Jedna destinacija po objektu — nema varijante
između petlji. AI Context je ispravljen na category noun + zonu (ne opis anomalije).

| Objekat | Kind / zona | Pomeraj |
|---|---|---|
| `SM_Chair_21` | an office chair / the meeting room with the long table | 288 cm |
| `SM_flower_pot_3` | a potted plant / beside the elevator doors | 738 cm |
| `SM_RubberMallet_A01_N1` | a rubber mallet / the tool shelf in the corner | 260 cm |
| `SM_ComputerMonitor_A02_N2` | a computer monitor / the back shelves past the lifts | 132 cm |

- [x] Destinacije autorovane (legacy `AnomalyLocation`, world space).
- [~] Nema 2–3 `AnomalyMovePoint` po objektu — radi, ali uvek ista destinacija.
- [x] Nijedna destinacija nije unutar 20 cm od rest poze.
- [x] `AnomalyZone` / `AnomalyObjectKind` popunjeni na sva četiri.

## 2. Material anomalije — audit (obavezno)

- [x] 10/10 `MaterialSwapAnomalyComponent` u `FullOfficeMap`: svaka varijanta je
  drugi asset od mesh defaulta (pamphlets I01/D01/F01 + `BP_OldMagazine` /
  `BP_OldMagazine2`). Nema praznog slota ni identičnog materijala.
- [x] `SelectionWeight` = 2.0 na svima. `AnomalyProbability` je 0.7 na većini;
  `I01_N1`, `F01_N1` i `D01_N2` stoje na 1.0 (editor override, češće pucaju).

## 3. Flashlight

Radi bez ikakvog editorskog posla — `F` na tastaturi, `Y`/`Triangle` na padu,
spot light je napravljen u C++ i zakačen na kameru.

- [x] `IA_Flashlight` (`/Game/MyStuff/Input/IA_Flashlight`) dodeljen na
  **Input > Flashlight Action** u `BP_Character`. `F` je u `IMC_Default`.
  Gamepad `Y`/`Triangle` i dalje ide kroz C++ fallback, da se ne duplira bind.
- [x] **Audio|Flashlight > Flashlight Toggle Sound** — `FlashlightToggle`
  (generisan klik). Fallback: elevator button press ako wav još nije uvezen.
- [ ] Proveri intenzitet u igri. C++ default je 2600 lm, cone 16°/34°, blago topla
  boja. Namerno ne briše mrak; ako je pretamno digni `Intensity` na komponenti
  `Flashlight` u BP-u karaktera.

## 4. Zvuk treperenja svetla

- [x] **Flicker Sound Attenuation** na `LightFlickerAnomalyComponent` — `ATT_FlickerHallway`
  (inner 300 cm, silent at 1800 cm) na `BP_Light_17`, `BP_Light_35`, `BP_Light_37`
  i kao C++ default. Detalji:
  [AUDIO_ASSIGNMENT_CHECKLIST.md](AUDIO_ASSIGNMENT_CHECKLIST.md#light-flicker-anomalije-lightflickeranomalycomponent).

## 5. Help ekran

Dugme **HELP** je pravi widget u `WBP_MainMenu` (redosled: Play, Settings,
Archive, Help, Quit). Tekst je lokalizovan i pisan u C++.

- [x] Pravo dugme imena `Help` (i `Archive`) postoji u `WBP_MainMenu`.
- [x] Sekcija o anomalijama je 25.08.2026. preokrenuta: sada piše **šta se NE
  računa** kao anomalija, uključujući brojač petlje, mrak, tvoju lampu, Dragojlov
  poziv i to što je jedan lift osvetljen. Stara lista devet tipova je izbačena.
- [ ] Ako želiš slike u Help ekranu: napravi WBP dete od `HelpWidget`, bindaj
  `VB_Sections` i postavi ga na **UI > Help Widget Class** na meniju. C++ i dalje
  ubacuje tekst, ti dodaješ vizual oko njega.

## 6. Dragojlo na main menu-u

- [x] Figura je namestena ručno u `DoFlicker` (spawn transform po oku, ka uglu
  a ne tačno pod lampom). Izgled je odobren 24.08.2026. **Ne vezivati** nod
  `Get Figure Transform Under Light` — to bi je vratilo pod svetlo.

Flicker i dalje živi u level Blueprintu mape `MainMenu` (`ScheduleFlicker` /
`DoFlicker`). Cilj treperenja je `BP_Light_1`, kamera je `CameraActor_0`
(tag `MenuCamera`). C++ helper ostaje kao Pure funkcija ako ikad zatreba,
ali trenutni look je hardcoded spawn.

## 7. Muzika u nivou — vlasnik se promenio (novo 25.08.2026.)

Muziku na spratu sada pušta `ALoop9GameMode` kao **2D** zvuk, isto kao što meni
pušta svoju. Postavljeni `AmbientSound_0` u `FullOfficeMap` se pri startu **utiša
namerno**, da se `HorrorAmbience1` ne bi svirao dva puta.

- [ ] Slobodno obriši `AmbientSound_0` iz `FullOfficeMap`. Nije obavezno — kod ga
  gasi sam — ali mapa je jasnija bez njega.
- [ ] Ako želiš drugi track ili drugu jačinu: **BP_Loop9GameMode > Audio > Level
  Music Sound / Level Music Volume**. Default je `HorrorAmbience1` na 0.35.
- [ ] Ostali `AmbientSound` akteri koji **nisu** na `SC_Music` se ne diraju, i
  dalje ih subsystem pokreće ako ne krenu sami.

---

## QA prolaz posle svega

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
