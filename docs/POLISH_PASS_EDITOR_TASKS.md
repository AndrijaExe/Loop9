# Polish pass — šta ostaje u editoru

Poslednje ažuriranje: **24.08.2026.**

C++ deo polish prolaza je odrađen i kompajlira se sam. Ono što je trebalo
u editoru (Move, materijali, atenuacija, Help/Archive dugmad, figura u meniju,
`IA_Flashlight`) je odrađeno. Gather Text je pokrenut 24.08.2026. Ostaje QA
u igri i opciono klik lampe.

Legenda: `[ ]` nije odrađeno · `[~]` radi ali vredi proveriti · `[x]` gotovo

---

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
Archive, Help, Quit). C++ `EnsureHelpButton()` se više ne klonira u runtime-u
jer `GetWidgetFromName("Help")` nalazi dugme. Tekst je lokalizovan i pisan u C++.

- [x] Pokreni meni i proveri da HELP stoji iznad QUIT i da se BACK vraća u meni.
- [x] Pravo dugme imena `Help` (i `Archive`) postoji u `WBP_MainMenu`.
- [ ] Ako želiš slike u Help ekranu: napravi WBP dete od `HelpWidget`, bindaj
  `VB_Sections` i postavi ga na **UI > Help Widget Class** na meniju. C++ i dalje
  ubacuje tekst, ti dodaješ vizual oko njega.
- [x] **Lokalizacija** je odrađena: svih 14 `Loop9Help,*` ključeva plus
  `Loop9Menu,Help` prevedeni su za `sr`, `de`, `fr`, `ru` i upisani direktno u
  PO fajlove. `msgid` je proveren znak po znak prema C++ izvoru, pa GatherText
  treba da ih prepozna i sačuva `msgstr`.
- [x] Ipak pokreni **Gather Text** pa **Compile Text** i posle toga potvrdi da
  prevodi nisu ispali. Ako neki `msgstr` postane prazan, znači da se `msgid`
  razlikuje — u tom slučaju je najlakše prekopirati tekst iz `en/Game.po`.

## 6. Dragojlo na main menu-u

- [x] Figura je namestena ručno u `DoFlicker` (spawn transform po oku, ka uglu
  a ne tačno pod lampom). Izgled je odobren 24.08.2026. **Ne vezivati** nod
  `Get Figure Transform Under Light` — to bi je vratilo pod svetlo.

Flicker i dalje živi u level Blueprintu mape `MainMenu` (`ScheduleFlicker` /
`DoFlicker`). Cilj treperenja je `BP_Light_1`, kamera je `CameraActor_0`
(tag `MenuCamera`). C++ helper ostaje kao Pure funkcija ako ikad zatreba,
ali trenutni look je hardcoded spawn.

---

## QA prolaz posle svega

- [ ] Odigraj 10 petlji i zabeleži koliko ih je imalo anomaliju. Očekivano ~8/10,
  loop 1 uvek čist.
- [ ] U tih 10 petlji: Hide i Material/Text treba da se pojave češće nego ranije,
  Pursuer osetno ređe.
- [ ] Nijedna aktivna anomalija ne sme biti nevidljiva. Ako se to opet desi,
  `AnomalyList` u konzoli pokazuje šta je aktivno na tom spratu.
- [ ] Background muzika radi **bez** ulaska u settings menu. Ovo je bio bug: mix se
  smatrao primenjenim i kad audio device još nije bio spreman, pa je ulazak u
  settings bio jedini način da se ponovo primeni.
- [ ] Zvuk treperenja se čuje samo blizu tog svetla, ne kroz celu mapu.
- [ ] `F` gasi i pali lampu, i ne radi tokom pauze i inspekcije objekta.
