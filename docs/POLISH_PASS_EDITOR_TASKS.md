# Polish pass — šta ostaje u editoru

Poslednje ažuriranje: **23.08.2026.**

C++ deo polish prolaza je odrađen i kompajlira se sam. Ovde je samo ono što
C++ ne može da uradi umesto tebe: postavljanje aktera u mapi, dodela asseta i
jedna izmena u level Blueprintu menija.

Legenda: `[ ]` nije odrađeno · `[~]` radi ali vredi proveriti · `[x]` gotovo

---

## 1. Move anomalije — spawn pointovi (obavezno)

Bez ovoga Move anomalije **neće da se aktiviraju** — namerno. Umesto da objekat
odu na koordinatu `(0,0,0)`, komponenta upiše warning u log i propusti se, pa
manager izvuče drugu anomaliju. To je ono što je uzrokovalo Dragojla na
nepostojećem trećem spratu.

- [ ] Za svaki objekat sa `MoveAnomalyComponent` postavi **2–3 `AnomalyMovePoint`**
  aktera tamo gde objekat sme da se pojavi. Strelica na akteru je rotacija koju
  objekat dobija, pa je usmeri kako objekat treba da stoji.
- [ ] Poveži ih na jedan od dva načina:
  - **Move Anomaly > Targets > Move Target Points** — nabroj ih direktno. Najjasnije
    za jedan objekat.
  - ili ostavi tu listu praznu, upiši **Move Target Tag** na komponenti i isti
    `Point Tag` na svaki point. Zgodno kad više objekata deli isti set mesta.
    Prazan tag = nema pretrage po levelu, pa jedan zaboravljen point ne može
    da preusmeri sve ostale Move anomalije na spratu.
- [ ] Proveri da nijedan point nije na mestu gde objekat već stoji (tolerancija je
  20 cm). Takav point se u runtime-u preskače, jer „pomeranje“ koje igrač ne vidi
  kažnjava igrača što je ispravno primetio da se ništa nije promenilo.
- [ ] Ako je neki `AnomalyLocation` već ručno autorovan i radi kako treba, ne mora
  ništa — koristi se kad nema pointova. Pointovi imaju prioritet.

## 2. Material anomalije — audit (obavezno)

- [ ] Uđi u `FullOfficeMap`, pokreni PIE i u konzoli izvrši **`AnomalyAuditMaterials`**.
  Ispisuje svaki material swap koji igrač ne bi mogao da vidi ni kad se aktivira
  (varijanta identična normalnom materijalu, prazan slot, nepostojeći mesh).
- [ ] Popravi sve što ispiše. Od sada takva anomalija odbija da se aktivira, tako
  da spratu ne propada anomalija — ali ti se troši placement koji ništa ne radi.
- [ ] Napomena o frekvenciji: `MaterialSwapAnomalyComponent` sada ima
  `AnomalyProbability` = 0.7 i `SelectionWeight` = 2.0 **u C++ konstruktoru**. Ako je
  neka instanca u mapi ranije override-ovala te vrednosti u editoru, ona ostaje na
  starom broju. Proveri par placementa i resetuj strelicom na property ako želiš
  nove defaulte.

## 3. Flashlight

Radi bez ikakvog editorskog posla — `F` na tastaturi, `Y`/`Triangle` na padu,
spot light je napravljen u C++ i zakačen na kameru.

- [ ] Opciono: napravi `IA_Flashlight` Input Action i dodeli ga na
  **Input > Flashlight Action**, pa dodaj `F` u `IMC_Default`. Tada ide kroz
  normalan input setup i pojavljuje se u rebind UI-u, umesto kroz runtime fallback.
- [ ] Opciono: **Audio|Flashlight > Flashlight Toggle Sound** — klik lampe.
- [ ] Proveri intenzitet u igri. C++ default je 2600 lm, cone 16°/34°, blago topla
  boja. Namerno ne briše mrak; ako je pretamno digni `Intensity` na komponenti
  `Flashlight` u BP-u karaktera.

## 4. Zvuk treperenja svetla

- [ ] **Flicker Sound Attenuation** na `LightFlickerAnomalyComponent` — jedina stvar
  koju treba dodeliti. Bez attenuation asseta zvuk se čuje sa cele mape, a cela
  poenta je da vodi igrača ka tom hodniku. Detalji i predložene vrednosti:
  [AUDIO_ASSIGNMENT_CHECKLIST.md](AUDIO_ASSIGNMENT_CHECKLIST.md#light-flicker-anomalije-lightflickeranomalycomponent).

## 5. Help ekran

Dugme **HELP** se pojavljuje samo od sebe: meni klonira Settings dugme u runtime-u
ako u WBP-u ne postoji dugme po imenu `Help`. Tekst je lokalizovan i pisan u C++.

- [ ] Pokreni meni i proveri da HELP stoji iznad QUIT i da se BACK vraća u meni.
- [ ] Kad sledeći put diraš `WBP_MainMenu`, napravi pravo dugme imena `Help` da
  runtime kloniranje više ne treba.
- [ ] Ako želiš slike u Help ekranu: napravi WBP dete od `HelpWidget`, bindaj
  `VB_Sections` i postavi ga na **UI > Help Widget Class** na meniju. C++ i dalje
  ubacuje tekst, ti dodaješ vizual oko njega.
- [x] **Lokalizacija** je odrađena: svih 14 `Loop9Help,*` ključeva plus
  `Loop9Menu,Help` prevedeni su za `sr`, `de`, `fr`, `ru` i upisani direktno u
  PO fajlove. `msgid` je proveren znak po znak prema C++ izvoru, pa GatherText
  treba da ih prepozna i sačuva `msgstr`.
- [ ] Ipak pokreni **Gather Text** pa **Compile Text** i posle toga potvrdi da
  prevodi nisu ispali. Ako neki `msgstr` postane prazan, znači da se `msgid`
  razlikuje — u tom slučaju je najlakše prekopirati tekst iz `en/Game.po`.

## 6. Dragojlo ispod svetla na main menu-u

Ovo je jedina stvar koju C++ ne može da odradi sam, jer flicker i spawn figure
žive u **level Blueprintu mape `MainMenu`** (funkcije `ScheduleFlicker` / `DoFlicker`,
promenljive `StartLoc` / `StartRot`), a ne u C++ direktoru.

Izmena je jedan nod:

- [ ] Otvori level Blueprint mape `MainMenu`.
- [ ] Nađi mesto gde se pravi `SpawnTransform` iz `StartLoc` / `StartRot` pre
  `SpawnActor` za `BP_PursuerAnomaly`.
- [ ] Zameni ga nodom **Get Figure Transform Under Light** (kategorija `Menu Scene`):
  - `Light Actor` → referenca svetla koje treperi (`SpotLight_5` ili `BP_Light_1_C_3`,
    ono koje `DoFlicker` gasi).
  - `Face Target` → ostavi prazno. Sam nađe aktera sa tagom `MenuCamera`, tj.
    `CameraActor_0`, i okrene figuru ka kameri.
  - `Floor Clearance` → 96 je capsule half height; ako figura upada u pod ili
    lebdi, ovo je broj koji se menja.
  - `Forward Offset` → 0 znači tačno pod svetlom. Pozitivna vrednost je gura ka
    kameri, ako centar ispod svetla izgleda previše pozirano.
- [ ] Izlazni `Transform` vodi direktno u `SpawnActor`.
- [ ] Nod traca nadole od **light komponente**, ne od pivota aktera, jer plafonska
  lampa često ima pivot u drugoj sobi. Ako figura ispadne na čudnom mestu, prvo
  proveri da si vezao pravo svetlo.
- [ ] `StartLoc` / `StartRot` posle ovoga mogu da se obrišu.

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
