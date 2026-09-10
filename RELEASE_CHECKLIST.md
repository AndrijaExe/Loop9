# Loop 9 — authoritative release checklist

Poslednje ažuriranje: **09.09.2026.**
Steam App ID: **4982260**

> **Valve build review je prošao (03.09.2026.).** Store spreman.
> Valve dozvoljava od **četvrtka 10.09.** Release day je **petak 11.09.2026.**
> Default je **v1.0.5** (`25175593`, Set Live 09.09.). Sledeći cook:
> **v1.0.6** — `ACH_SPOT_LOOPNUMBER` (treperenje brojača petlje) + Almanac 10/10.
> `valvereview` lozinka promenjena 07.09.
>
> **Ostalo pre 11.09.:**
> 1. Steamworks: unesi i **Publish** `ACH_SPOT_LOOPNUMBER` (Lost Count) pre
>    nego što v1.0.6 klijent krene da piše 28 imena. Ikonice: kopiraj sa
>    drugog `ACH_SPOT_*`. Almanac progress max 9 → 10 ako si ga imao.
> 2. Cook/upload **v1.0.6**, Set Live na **default** (streameri već snimaju).
> 3. Release day §8 u petak. Creator Homepage nije bloker.
>
> **Andrija — ručne stavke od 08.09. (ništa od ovoga nije u kodu):**
> - [ ] **Steamworks (pre v1.0.6 uploada):** New Achievement
>   `ACH_SPOT_LOOPNUMBER` — Display **Lost Count**, Description **Correctly
>   call out a flickering loop counter.** Hidden = No. Save + **Publish**.
>   Ako Almanac ima progress stat, max 9 → 10.
> - [ ] Render env: dodaj `AI_COMMITMENT_WRONG_LIFT_CHANCE=0.5` (backend je
>   deployovan; bez ključa važi `.env` = 0.5, radi i bez toga).
> - [ ] **OpenAI / provider nalog: postavi tvrdi mesečni spend limit.** Kod
>   ograničava broj zahteva (5000/dan globalno), ne dolare — ovo je poslednja
>   brana i jedina koju repo ne može da uradi za tebe.
> - [ ] Odluči: v1.0.6 pre petka (Pursuer mrtva linija + poruka ≤1000 znakova)
>   ili prvi patch posle launcha. Ako kuvaš: **GatherText** pa cook.
> - [ ] Ako kuvaš v1.0.6: Standalone QA Pursuer linije (§6) i jedan run da
>   chat radi normalno na sledećem spratu.
> - [ ] Monitoring alarm na `chat.denied.global` i `abuse.watch` (metrike već
>   postoje na `/metrics`). Ako launch dan probije 5000 legitimnih chatova,
>   igrači dobijaju „Service is at capacity“ — tada se diže
>   `GAME_GLOBAL_DAILY_QUOTA` na Renderu, ne unapred.

Ovo je jedini dokument koji prati spremnost za release. Tehničke tabele ostaju u
`[STEAM_ACHIEVEMENTS.md](STEAM_ACHIEVEMENTS.md)`, marketinški tekst u
`[Marketing/Steam/STORE_PAGE.md](Marketing/Steam/STORE_PAGE.md)`, a cinematic
dokumentacija u `[docs/CINEMATICS_AND_AUDIO.md](docs/CINEMATICS_AND_AUDIO.md)`.
Zvukovi za dodelu: `[docs/AUDIO_ASSIGNMENT_CHECKLIST.md](docs/AUDIO_ASSIGNMENT_CHECKLIST.md)`.
Otvoreni editorski ostaci polish prolaza:
`[docs/POLISH_PASS_EDITOR_TASKS.md](docs/POLISH_PASS_EDITOR_TASKS.md)`.

Legenda: `[ ]` nije gotovo · `[~]` podešeno, ali nije završno verifikovano ·
`[x]` završeno i potvrđeno

**Grane od 08.09.2026. (isto u oba repoa, `Loop9` i `Loop9_backend`):**

| Grana / tag | Šta znači | Ko sme da push-uje |
|---|---|---|
| `main` | Ono što svi igraju: Steam default build + Render deploy (backend push na `main` = live deploy) | Samo merge iz `develop` kad je feature gotov i QA-ovan, ili hotfix |
| `release/v1.0.5` | Rollback. Zamrznuta kopija `main`-a od 08.09. Ne dira se. | Niko |
| `develop` | Sav novi rad (1.1: Dragojlo memorija, „Loop 1“ ending, nove anomalije) | Slobodno |
| tag `v1.0.5-cook` (igra) | Tačan commit skuvan 07.09. (`c47c388`) — `main` posle njega ima Pursuer mrtvu liniju i cap poruke koji **nisu** u cooku | — |
| tag `launch-v1.0.5` (backend) | Backend kakav je na Renderu za launch | — |

Hotfix posle launcha: grana od `main`, merge u `main` **i** u `develop`.
GitHub branch protection na `main` (bez direktnog push-a) — ručno u
Settings → Branches, `gh` ovde nije prijavljen.

**Važno o dva stanja.** `[x]` ispod znači da je stvar u `main` repou, ne
nužno i na Steam playtestu. **`Builds/v1.0.0`** je trenutni fallback Shipping
drop — ne overwrite-ovati. WIP cook (novi featurei / bugfix) ide u
**`Builds/Feature`**. Nova verzija (`v1.0.1` itd.) dobija **novi folder** i tamo
se kuva. SteamPipe contentroot je **`Builds/v1.0.5/Windows`**. **`Builds/v1.0.0`
ostaje fallback** i ne overwrite-uje se. Stari `Builds/Alfa` i `Builds/Beta`
više nisu cilj. Live/default Shipping build je **v1.0.2** (BuildID
`25008533`, 29.08.2026) dok se Set Live ne uradi na `v1.0.5`. Development
reviewer build je `25008639` na `valvereview` (lozinka promenjena 07.09.).
Valve-oboreni cook je `24910264`.
**Coming Soon store je javan od 27.08.2026.**:
https://store.steampowered.com/app/4982260/Loop_9/

---

## 1. Content lock — završiti pre finalnog QA builda

- [x] Implementiran C++ elevator transition coordinator:
  decision deduplication, vrata, input/pause lock, blackout teleport, osvetljeni
  arrival lift, watchdog fallback i direktan C++ camera blend.
- [x] U `FullOfficeMap` povezani director, oba dugmeta, door wing reference i
  `TP_LitElevatorArrival`; lift više ne zavisi od Level Sequence asseta.
- [x] **Bug:** dugme lifta se može pritisnuti i kada igrač nije u kabini —
  C++ gate u `ALiftButton` (door-plane depth + distance; optional `CabinVolume`).
  U editoru po želji uvećati `CabinVolume` box extent za stroži overlap check.

- [x] U `BP_LoopElevatorTransitionDirector` postavljeni `Button Press Sound`
(`Sound/Elevator/ElevatorButtonPress`) i looping `Travel Sound`
(`Sound/Elevator/ElevatorTravelLoop`, `Looping` = true na SoundWave-u) — i na
class defaults i na instancu u `FullOfficeMap`. Zvukovi su uvezeni iz CC0
izvora; licence i izvorni URL-ovi su u
`[docs/AUDIO_ASSIGNMENT_CHECKLIST.md](docs/AUDIO_ASSIGNMENT_CHECKLIST.md)`
(atribucija nije potrebna). Volume/fade ostaju na defaultima
(`1.0 / 1.0 / 0.15 s`). Mix i da travel loop ne slaže vožnje potvrđeni
na **v1.0.3** polish smoke (04.09.2026.).

- [x] Obrisani `LS_Elevator_Lit` i `LS_Elevator_Dark`. Pre brisanja potvrđeno
  nulom referenci (asset registry) i binarnom proverom da ih ni
  `BP_LoopElevatorTransitionDirector` ni `FullOfficeMap` više ne pominju.
- [x] Implementiran C++ ending Sequence player sa šest soft-reference slotova,
  watchdogom i postojećim fade/widget fallbackom.
- [x] Napravljeno i povezano šest osnovnih 3–8 s ending Level Sequence asseta.

- [~] Vizuelno i zvučno dotegnuti svih šest ending mini-sekvenci: kamera,
svetlo, sitne prop animacije i završni prelaz u postojeći widget/terminal.
Detaljan Unreal MCP handoff:
`[docs/UNREAL_MCP_ENDING_SCENES_HANDOFF.md](docs/UNREAL_MCP_ENDING_SCENES_HANDOFF.md)`.
Napomena: kadar i svetlo vodi C++ `ALoopEndingSceneDirector`, ne Sequencer keyevi.
Slotovi na instanci u `FullOfficeMap` su popunjeni (phone, chair, arrival point,
tri dimmable RectLighta, companion class, footstep/phone-ring/flicker zvuk,
i `Line Cut Sound` = `Sound/Phone/PhoneLineCut` za Cold Betrayal).

- [x] **Bug:** prazan `Cold Betrayal Door Wings` je terao `GatherColdBetrayalDoors()`
  na fallback koji hvata *sve* `ALiftDoorWing` aktere u mapi (20+), pa je beat
  čekao da se otvore i lift vrata koja nemaju veze sa dolaskom. Slot je sada
  vezan na ista arrival krila kao elevator director (`BP_LiftDoorWing_C_3/4`).


### Ending mini-scene brief

Koristiti jednu zajedničku 4–6 s baznu sekvencu i duplirati je šest puta.
Menjati samo camera transform, svetlo, 1–2 prop keyframea i zvuk; bez novih
skeletal animacija i bez gameplay logike u Event Tracku.

- [x] **Escape Together:** kroz već otvorena arrival vrata raste toplo jako
  svetlo; čuju se dva para koraka; drugi lift se uključi neposredno pre kraja.
- [x] **Obedient Fool:** kamera prilazi telefonu; svetla iza igrača gase se jedno
  po jedno; monitor prikazuje `TASK COMPLETE`; telefon kratko zazvoni.
- [x] **Cold Betrayal:** vrata otkrivaju potpuno mračan hodnik; čuje se prekinuta
  telefonska linija; pali se crveno svetlo i vrata se ponovo zatvaraju.
- [x] **Bug:** Cold Betrayal — posle blackouta oči (dve crvene tačke) se uopšte
  ne vide; vratiti vidljiv dual-eye beat (emissive mesh ili pouzdan light setup)
  tek kada su svetla skroz ugašena, sa holdom ~1 s dužim od ranije.
- [x] **Paranoid Survivor:** kamera se okreće od telefona ka izlazu; telefon
  zazvoni iza igrača; kadar počinje da se vraća, ali se završava pre otkrivanja.
- [x] **Merged Memory:** fluorescentna svetla trepere; telefon/monitor se kratko
  pojavljuje na dve pozicije; duplirani zvuk kasni nekoliko desetina sekunde;
  kamera polako prilazi monitoru.
- [x] **The Replacement:** kamera prilazi Dragojlovom stolu; prazna stolica se
  blago okreće; telefon zazvoni; monitor prikazuje `NEW OPERATOR CONNECTED`;
  zatim se prikazuje postojeći Replacement terminal.


### Ending balance targets

Pragovi su scoring od 01.09.2026., podešeni za prvi prolaz od približno 9–17
odluka, bez znanja skrivenih ključnih reči. Kod i fixture testovi su u `main`;
prirodni runovi u igri još nisu potvrđeni.

- [x] **Paranoid Survivor:** 0–2 AI razgovora ili visok suspicion / nizak trust.
  Namerni ending za igrača koji ignoriše Dragojla. Kod: tvrda kapija `< 3`
  chatova, inače scoring.
- [x] **Escape Together:** 4–7 razgovora, pristojnost, niska zavisnost, stabilan
  AI. First-run good ending. Scoring, ne AND kapija 0.62/0.60/0.70.
- [x] **Cold Betrayal:** ≥4 razgovora, prati ga, ali ~dve uvredljive poruke
  (nizak kindness). Više ne traži kindness ≤ 0.40 kao tvrdu kapiju.
- [x] **Obedient Fool:** često predaje odluku (`DEPENDENCY=1`). Dep prag ~0.44
  umesto 0.53; 6+ chatova umesto 8.
- [x] **Merged Memory:** ostao je ljubazan, run je neredan (stability ~0.74 ili
  niže). Više ne traži ≤ 0.72 i ≥9 advances kao AND.
- [x] **The Replacement:** dugačak clingy run, visok dep, nestabilan AI, 9+
  chatova. Retko, ali dostižno bez 11 chatova i dep 0.62.
- [x] Evaluator od 01.09.2026. **skoruje svih šest** i bira najbliži profil.
  Nema waterfall fallback na Paranoid. Testovi:
  `Loop9.Runtime.Endings.EvaluatorProfiles` (setup fixturei + near-miss runovi).
- [ ] QA u igri: po jedan prirodan run za Cold / Obedient / Merged / Replacement
  na novom cooku. `EndingSetup` i dalje 1:1.
- [ ] QA dependency: na bilo kom jeziku, predaja odluke Dragojlu diže
  `[STATE]DEPENDENCY=1` i gura ka Obedient Fool; samostalna odluka spušta.


### Runtime hardening (urađeno pre 21.08)

- [x] Rebuildovati `Loop9Editor` posle C++ izmena od 22–25.08 (Help, lampa,
  vrata, typewriter, muzika sprata, dead-code cleanup). Potvrđeno 03.09.
  (`ObservationZoneVolume` compile na kućnom Windowsu).
- [x] U tracked `Config/DefaultEngine.ini` postavljeno:
  - `SteamDevAppId=4982260`
  - `r.VirtualTextures=True`
- [x] Tracked production konfiguracija sada uključuje startup/cook mape, svih pet
  kultura i javni backend endpoint; nijedan API ključ/game token nije upakovan.
- [x] Gameplay-state hardening implementiran:
  prvi sprat ostaje clean baseline i posle reseta, AI limit se resetuje po poseti,
  a anomaly snapshot/repeat tracking više ne ostaje stale na ponovljenom spratu.
- [x] Anomaly lifecycle hardening implementiran:
  Pursuer ostaje zabeležen do odluke, Hide vraća originalni transform/visibility/
  collision, a looping audio se gasi pri resetu i world teardownu.
- [x] AI/ending state hardening implementiran:
  samo validiran AI odgovor menja relationship statistike, neuspešan ending
  presenter ne zaključava gameplay i prethodni movement mode se vraća posle chata.
- [x] Steam achievement upisi imaju GameInstance pending queue i capped exponential
  retry koji preživljava promenu nivoa i kasno pojavljivanje Steam identiteta.
- [x] UI/runtime cleanup implementiran:
  stale notification/widget timeri se čiste, main/settings meni fokusira pravo
  dugme za gamepad/tastaturu, a telemetry razlikuje transport uspeh od HTTP 2xx.
- [x] GatherText od **21.08.2026.** — tada su `de/fr/ru/sr` PO i `.locres`
  bili usklađeni, plus smoke svih pet jezika.
- [x] **Ponovo GatherText 26.08.2026. uveče.** `HOW TO PLAY` / `CREDITS` i ostali
  novi stringovi su u `.locres`.
- [x] **GatherText 28.08.2026.** `CreditsEngineHeading` (`ENGINE`) je u PO,
  archive, manifest i `.locres`. Telo UE EULA notice-a ostaje `FText::FromString`.
- [x] Item inspection smoke:
  otvaranje/zatvaranje, rotacija 15 s, bez ljubičastih artefakata, povratak inputa
  i pause menija.
- [x] MaterialSwap smoke za Magazine, I01, F01 i D01; potvrđeno vraćanje originalnog
  materijala na sledećem loopu.


### Polish pass u `main` (22–25.08.2026.) — u v1.0.1 playtestu

C++ i editorski deo (Move destinacije, materijali, atenuacija, Help/Archive
dugmad, figura u meniju, `IA_Flashlight`) su odrađeni. Sitnice koje još stoje
u editoru: `[docs/POLISH_PASS_EDITOR_TASKS.md](docs/POLISH_PASS_EDITOR_TASKS.md)`.

- [x] Anomalija na ~80 % petlji, 20 % čistih; loop 1 i dalje uvek čist
  (`AnomalyChancePerLoop = 0.8`).
- [x] Hide i Material/Text češći, Pursuer ređi.
- [x] Dragojlo ima 2–3 fiksne spawn tačke umesto random poda koji ne postoji.
- [x] Material anomalije vidljivije i češće.
- [x] Zeleni telefon i flicker najbliži liftu pomerani dublje u mapu da se
  ne čuju iz kabine.
- [x] Zvuk treperenja na poziciji svetla, sa `ATT_FlickerHallway` atenuacijom
  (C++ default, ne per-instance assignment).
- [x] Baterijska lampa na `F` (gasi se tokom pauze i inspekcije). Default
  2600 lm, cone 16°/34°, namerno ne briše mrak.
- [x] Help na main meniju: šta **nije** anomalija, dva lifta, broj petlje
  nije anomalija; C++ puni tekst, WBP može da doda slike.
- [x] Lore zapisano u `[docs/LORE.md](docs/LORE.md)`.
- [x] Pursuer pušta napetiju muziku dok juri; ambient se vraća kad nestane
  (popravljen brojač utišavanja koji je sprat ostavljao tih do kraja sesije).
- [x] Background muzika sprata sada ide iz `ALoop9GameMode` kao 2D bed, ne iz
  `AmbientSound_0`. Kod namerno utiša stari akter da se `HorrorAmbience1` ne
  duplira. Konzola: `AudioStatus`.
- [x] Ending widget kuca tekst (~20 znakova/s) sa greškom koju „ispravi“.
  Dugme piše **SKIP** dok kuca (prvi klik dopuni tekst), pa **Return to Main Menu**.
- [x] Vrata: locked / blocked / open / close zvukovi + blagi shake na zaključanim
  (2°, 0.35 s). C++ traži assete po imenu i ne puca ako fale.
- [x] Session timeline na ending ekranu i Archive na meniju čitaju `SeenEndings`.
- [x] Dead code očišćen; svi player-facing stringovi prevedeni u PO za 5 jezika.

Ostaje u editoru pre sledećeg Shipping builda:

- [x] Uvezi `Content/MyStuff/Sound/UI/TypewriterKey.wav` kao `SoundWave`
  `/Game/MyStuff/Sound/UI/TypewriterKey`, Sound Class **`SC_SFX`**.
- [x] Uvezi četiri `Content/MyStuff/Sound/Doors/*.wav` (`DoorOpen`, `DoorClose`,
  `DoorLocked`, `DoorBlocked`). `.uasset` postoje od 25.08.2026.
- [x] GatherText — urađen 25.08.2026.; `.locres` je u ovom commitu.
- [x] Proveri intenzitet lampe u BP karaktera — u redu, ne menjati (26.08.2026.).
- [x] Obriši `AmbientSound_0` iz `FullOfficeMap` — urađeno 26.08.2026. uveče
  (svirao `HorrorAmbience1`).
- [x] Help slike — ne radimo za sada (26.08.2026.).
- [ ] Opciono: 2–3 `AnomalyMovePoint` po Move objektu ako pomeraj deluje
  previše isti svaku petlju.

Sledeća sesija (urađeno 26.08.2026.):

- [x] Replacement terminal: isti typewriter zvuk kao ending WBP
  (`TypewriterKey`) i **SKIP** tokom ispisa (prvi klik dovrši tekst, ne meni).
- [x] Credits odvojeni od Help-a: `CreditsWidget` + dugme na meniju
  (Play → How to Play → Settings → Archive → Credits → Quit). Help više
  ne nosi SOUND CREDITS. Freesound autori iz download istorije su u
  Credits i `STORE_PAGE.md` Legal (26.08.). Steamworks Legal / About
  usklađen 26.08.2026.
- [x] Main-menu dugme `HELP` preimenovano u **HOW TO PLAY**.
- [x] GatherText posle ovih stringova da `HOW TO PLAY` / `CREDITS` uđu u
  `.locres` (26.08.2026. uveče).
- [x] **Raspored dugmadi (28.08. / 07.09.):** `Return to Main Menu` na ending
  ekranima ide donji desni ugao (`AlignToCanvasBottomRight`, svih šest WBP +
  replacement terminal). Main-menu VerticalBox podignut; v1.0.3 smoke 04.09.
  i editor 07.09.


## 2. Steamworks — Store Presence

- [x] App kreiran; App ID je `4982260`.
- [x] Basic Info, platforma, jezici, žanrovi, features i launch option
  (`Loop9.exe`) popunjeni.
- [x] **Installation → Common Redistributables:** `DirectX End-User Runtimes
  (June 2010)` i `Microsoft Visual C++ Redistributable 2022` čekirani i
  publish-ovani **28.08.2026.** Valve build review **prošao 03.09.2026.**
- [x] Passworded grana `valvereview` sa Development buildom **`25008639`**
  (29.08.2026., `UploadValvereview.bat`, setlive valvereview). Playtest /
  live default je Shipping **`25008533`**. Lozinka promenjena **07.09.2026.**
  posle odobrenja. Lozinka **nije** u gitu.
- [x] Content Survey popunjen sa runtime AI i AI-assisted marketing disclosure.
- [x] EN/SR store opis pripremljen.

- [x] DE/FR/RU lokalizovani opisi su uneti i sačuvani u Steamworksu; izvor je
`Marketing/Steam/STORE_PAGE.md`.

- [x] Cena `$4.99` i Valve regional pricing poslati.
- [x] Sačekati potvrdu pricing promena.
- [x] Steam Cloud: Properties → General pokazuje **108 bytes** na
  **`25008533`** (29.08. 12:52). Fajl je Steam Remote Storage
  `userdata\...\4982260\remote\Game.ini`. Auto-Cloud i dalje briše AppData
  `Game.ini`; Valveova zamerka je bila Properties, to je zatvoreno.
  `25008533` je live na defaultu. Two-machine round-trip nije rađen.
- [x] **„Enable cloud support for developers only"** (Cloud → Beta Testing)
  provereno 28.08. — **nije bilo čekirano**, pa nije uzrok.
- [x] Putanja potvrđena: `%LOCALAPPDATA%\Loop9\Saved\Config\Windows\`. Fajl
  **nije** pored `.exe`. Root ostaje `WinAppDataLocal`. Ne prebacuj na
  `gameinstall`. Detalji: `VALVE_REVIEW_REPLY.md` §2.
- [x] Odlučeno da `GameUserSettings.ini` **ne** ide na Auto-Cloud. Steamov
  best-practice traži da se izbegne machine-specific config.
- [x] **Cloud se verifikuje samo na Shipping buildu.** Development path pored
  `.exe` nije merodavan. Valve zamerka na Cloud zatvorena review-om 03.09.
  Two-machine round-trip i dalje nije rađen (§6).
- [x] Javni privacy policy postoji na backendu.
- [x] `https://loop9-backend.onrender.com/privacy` vraća HTTP 200
  (ponovo provereno 07.08.2026.).

- [x] Content Survey i Steamworks promene provereni u **Publish** tabu; izmene su
publish-ovane, ne samo sačuvane.

### Store grafika

- [x] Finalizovati 4 obavezne Store kapsule:
  `920×430`, `462×174`, `1232×706`, `748×896`.
- [x] Uploadovati minimum 5 stvarnih 16:9 gameplay screenshotova
  (`1920×1080` ili više); preporuka je 8 kadrova.
- [x] Finalizovati 4 Library asseta:
  `600×900`, `920×430`, `3840×1240` hero bez teksta i transparentni logo.
- [x] Shortcut ICO i App Icon JPG spremni u `Marketing/Steam/ClientAssets/`
  (lift + ruka, 01.09.2026.). Ako Steam klijent još pokazuje staru ikonicu,
  uploadovati `loop9_shortcut.ico` i `loop9_app_icon_184.jpg` u Steamworks.
- [x] Dodati opcioni Page Background `1438×810`.
- [x] Gameplay trailer snimljen, montiran i uploadovan u Steamworks
  (Store Presence → Trailers + Publish). Valve store review je **odobren**;
  Coming Soon stranica je javna od **27.08.2026.** Thumbnail je u
  `Marketing/Steam/StoreUpload/trailer_thumbnail_1920x1080.jpg`.
- [ ] Creator Homepage može posle Coming Soon stranice; nije release bloker.


### Branding u igri — ikonice, splash i boot sekvenca

Steamworks store grafika je gotova. Ikonica i splash su u **v1.0.3** cooku
(04.09.2026.). Stari `v1.0.0` / `v1.0.1` i dalje nose Epic identitet.

- [x] **Ikonica `.exe`-a.** Master i ICO (lift, ruka, crveno 9) su u
  `Marketing/Steam/ClientAssets/`. Kopirano u `Build/Windows/Application.ico`
  pre v1.0.3 cooka. Potvrđeno na paketovanom buildu 04.09.2026.
- [x] **Splash pri pokretanju.** `Content/Splash/Splash.bmp` — mali prozor
  **720×480** (ne 1920×1080 fullscreen). Otvoren lift, crveno 9, monstera,
  LOOP 9 + SMENA u donjem desnom. Potvrđeno na v1.0.3 04.09.2026.
  Izvor: `Marketing/Branding/loop9_splash_source.png`. `EdSplash.bmp` nije
  stavljen.
- [x] Napomena: ikonica Unreal Engine-a u **editoru** je `UnrealEditor.exe`,
  ne igra. Na paketovanom v1.0.3 je SMENA/lift ikonica.
- [x] **Ime studija i wordmark.** SMENA („smena“ = radna smena), izabran
  28.08.2026. Žig je outline-ovan u SVG putanje pa ne zavisi od fonta; svi
  fajlovi i objašnjenje su u
  `[Marketing/Branding/](Marketing/Branding/README.md)`.
- [x] **Provera kolizije imena — 28.08., nije pravni savet.** Steam: nema
  publishera „SMENA“. USPTO: mrtav Class 25 `79311908`. EUIPO eSearch:
  ESMENA (MECALUX, 20/39 i 6/20/39) skladišta; SMENA MPSHO Class **25**
  odeća; SEÑORÍO DE ESMENA Class 33 povučen; NESMENA Class 5 surrendered;
  **rosmena** `018814902` Class **9** (druga reč, software); **smena.**
  `018941455` Emilia Jansson, Class **19/27/28** — Class 28 uključuje igre/
  igračke, najbliži živi konflikt. Nema EUTM „SMENA“ u Class 9/41. Možeš
  staviti SMENA u Developer/Publisher; `smena.` Class 28 nije hard block za
  studio ime uz naslov Loop 9, ali nije nula. Ne uzimaj `smena.studio` domen.
- [x] **Developer / Publisher = SMENA.** Steamworks polja postavio Andrija
  01.09.2026. `STORE_PAGE.md` usklađen. `DefaultGame.ini` `CompanyName=SMENA`.
  Krediti: nova `STUDIO` sekcija („A SMENA game / Created by Andrija
  Stanišić"). GatherText za `STUDIO` još treba.
- [ ] **Boot logo sekvenca** (studio žig pre menija). Sad je odblokirana, ime
  postoji. Preporuka je UMG animacija u postojećem terminal / typewriter jeziku
  igre, 2–3 sekunde, skip na bilo koji input, ne startup `.mp4`. Konkretno:
  kursor kucka, `SMENA` se iskuca, kursor ostane da blinka, fade. Scanline
  tretman (varijanta 06 u `wordmark-explorations.png`) je dobar kao *prelaz*,
  ne kao završni kadar.
- [x] **Ne dodavati Unreal Engine logo u boot sekvencu.** Po Epicu, prikaz UE
  logotipa traži posebnu dozvolu preko branding zahteva; AAA igre koje ga
  prikazuju imaju custom licencu. EULA zahteva samo tekst u kreditima.
  Držimo se toga: samo `ENGINE` tekst u Credits.
- [x] **UE atribucija u kreditima (obavezna po EULA).** Dodata `ENGINE` sekcija
  u `CreditsWidget` 28.08.2026. sa tačnom formulacijom koju Epic traži.
  Namerno nije lokalizovana da prevod ne bi izmenio pravni tekst.
- [x] GatherText 28.08.2026. — `ENGINE` naslov je u `.locres`. Telo sekcije je
  `FText::FromString` i namerno se ne gather-uje.
- [ ] GatherText za novu `STUDIO` sekciju da uđe u `.locres`.


## 3. Steam achievements

Dva spremišta, od 25.08.2026. spojena pri čitanju. Nijedno **nije** namerno
privremeno na playtestu.

| Šta vidiš | Gde živi | Treba da preživi restart? |
|---|---|---|
| Steam toast / overlay lista | Steam nalog, App ID `4982260` | Da. Isto pre i posle release-a, čim su Stats & Achievements publish-ovani. |
| Archive / „koje endinge sam video“ | `%LOCALAPPDATA%/Loop9/Saved/Config/Windows/Game.ini` → `[/Script/Loop9.Loop9AchievementsSubsystem]` `SeenEndings` | Da, i sada se dopunjava iz Steama. |

Playtest *branch* na istom App ID-u deli iste achievemente kao `default`.

- [x] **Bug (nađen 25.08.2026., popravljen u `main`, nije u v1.0.0):** nijedan
  achievement se nikad nije otključavao — video se samo progress toast
  („Sharp Eye 6/7“) pa ništa. Dva različita puta u kodu: progress ide
  direktnim Steamworks pozivom (`FLoop9SteamUtils::IndicateAchievementProgress`)
  i radio je, a unlock je išao preko Online Subsystema, koji u UE odbija
  **svaki** read i write ako achievementi nisu popisani u `DefaultEngine.ini`
  pod `[OnlineSubsystemSteam]` kao `Achievement_N_Id=...`. Tog bloka nije bilo,
  pa je `QueryAchievements` padao, `bCacheReady` nikad nije postao `true` i
  `FlushPendingUnlocks` se nikad nije ni pozvao. Popravka je dvostruka:
  svih 27 imena je upisano u config (redom od 0, bez navodnika), a
  `UnlockAchievement` sada prvo zove Steamworks direktno
  (`SetAchievement` + `StoreStats`), pa tek onda pada na subsystem. Direktan
  put ne traži ni keširanu listu ni online identitet, pa radi i ako engine
  opet ne pročita config.
- [x] Verifikovati popravku na Windows buildu: unlock i toast rade, achievementi
  se čuvaju. Potvrđeno 04.09.2026. (v1.0.3 / Steam).

- [x] **Arhiva se sama popravlja (25.08.2026.):** `GetSeenEndingIds` i
  `RecordSpottedAnomalies` više ne čitaju samo `Game.ini`, nego ga spajaju sa
  onim što Steam drži za šest `ACH_ENDING_*` i devet `ACH_SPOT_*`. Isti podatak,
  dva izvora: Steam preživi izgubljen ili Cloudom pregažen fajl, a fajl radi
  offline i bez Steama, gde Steam ne vraća ništa. Promašaj sa Steama se nikad ne
  čita kao „nije otključano“, pa nema lažnog brisanja. Spojena lista se upiše
  natrag samo kad se dve razlikuju. Time Cloud prestaje da bude nosiva greda za
  arhivu i za `ACH_SPOT_ALL`.
- [x] Provera arhive / persist: achievementi i viđeni endingi se čuvaju.
  Potvrđeno 04.09.2026. Two-machine Cloud round-trip i dalje nije rađen.

- [x] Definisano svih **27** API imena tačno po
  `[STEAM_ACHIEVEMENTS.md](STEAM_ACHIEVEMENTS.md)`.
- [x] Uploadovano 27 achieved + 27 locked ikonica.
- [x] Postaviti hidden flag za 6 endinga, `ACH_DEJA_VU` i
  `ACH_SPOT_PHANTOM`.
- [x] Publishovati Stats & Achievements promene.
- [ ] Testirati najmanje po jedan achievement iz svake grupe, zatim svih šest
  endinga i `ACH_SPOT_ALL` sa 9 anomaly tipova. **Ovo do 25.08. nije moglo da
  prođe** zbog buga iznad; ponoviti od nule na novom buildu.
- [ ] **Persist QA:** otključaj jedan achievement i jedan ending → potpuno
  izađi iz igre i Steama → ponovo iz Library-ja. Overlay i dalje pokazuje
  unlock; `Game.ini` i dalje ima taj `SeenEndings` red; Archive (na novom
  buildu) i dalje pokazuje čvor. Ako Steam lista padne a `Game.ini` ostane,
  to je Steam stats, ne save. Ako padne `Game.ini`, prvo Cloud konflikt.
- [ ] Ako neki achievement i posle popravke ne pukne, proveri da li je to ime
  publish-ovano u Steamworksu. Direktan `SetAchievement` tiho ne uspeva za
  ime koje Steam ne poznaje, a `ACH_SPOT_*` imena moraju da se slažu do slova.


## 4. Backend / production

- [x] Production je Steam-only; legacy game token je onemogućen u `prod`.
- [x] `/readyz`, Redis limiter storage, request bounds, AI deadline, moderation,
  response-size limit i correlation/timing logovi su implementirani.
- [x] Lokalni production QA sa dva backend replica procesa i zajedničkim Redisom
  potvrdio je atomske auth/chat/IP/player/monthly/global limite; zahtevi odbijeni
  globalnim limitom ne stižu do moderation/chat provajdera.
- [x] Privacy policy endpoint je implementiran i pushovan.
- [x] Compact/full AI promptovi su usklađeni sa devet anomaly tipova, clean
  prvim loopom, ending relationship tonom i preciznim KINDNESS/SUSPICION rubricima.
- [x] Backend normalizuje interne Unreal anomaly oznake pre slanja modelu i ima
  lokalizovane moderation fallback poruke za EN/SR/DE/FR/RU.
- [x] Prompt QA: clean prvi loop uvek preporučuje dark lift; proveriti po jedan
  Hide/Light/Phantom kontekst i neutralan/ljubazan/sumnjičav input na svih pet jezika.
- [x] Backend traži od modela da prizna da ne zna gde je anomalija umesto da
  izmisli mesto; sa popunjenim `AnomalyZone`/`AnomalyObjectKind` ume da pokaže deo
  sprata bez odavanja predmeta, srazmerno poverenju igrača. §9 ima editor deo.

- [x] Render env za parove modela: `AI_MODEL=gpt-5.6-terra` (tier `best`, otvara
  petlje 4+) i `AI_FALLBACK2_MODEL=gpt-5.6-luna` (tier `cheap`, otvara petlje 1–3),
  uz `AI_FALLBACK2_ENABLED=true`, URL i ključ. **Prazan** `AI_FALLBACK2_API_KEY` **tiho
  izbacuje ceo cheap tier** i sve petlje idu na primary, bez ijedne greške u logu.
- [x] Potvrditi koji je `AI_MODEL` zaista aktivan na Renderu. Ako promenljiva tamo
  nije postavljena, važi commitovani `.env` default, pa se model menja samim
  deployom a ne svesnom odlukom.
- [x] Ponoviti prompt QA za čist sprat i na kasnijoj petlji, ne samo na prvoj.
  Klijent za čist sprat šalje `anomaly_key="none"`, što je backend do 20.08.2026.
  čitao kao aktivnu anomaliju i forsirao osvetljeni lift tamo gde je mračni tačan.

- [x] `STEAM_APP_ID=4982260` i `STEAM_WEB_API_KEY` potvrđeni pravim auth zahtevom:
playtest build pokrenut iz Steam Library-ja dobio je ticket, sesiju i žive AI
odgovore (17.08.2026). Backend je od tada na **Starter** (always-on) planu.

- [x] Nevalidan `AI_MODERATION_API_KEY` vraća HTTP 200, ali svaki odgovor postaje
  ista in-fiction fallback rečenica i chat provajder se nikad ne pozove. Kad
  „AI ne radi“ a igra ne prijavljuje grešku, prvo proveriti moderation ključ i
  `Content safety decision.` u logu.

- [x] Javni `https://loop9-backend.onrender.com/readyz` vraća
  `{"status":"ready"}` (provereno 07.08.2026.).
- [ ] Render Health Check Path postaviti/potvrditi kao `/readyz`.
- [x] Pre javnog release-a ukloniti cold start: Render Starter ili ekvivalentan
  always-on plan. Spoljni keep-alive free servisa nije pouzdan production plan.
- [ ] Posle prelaska na always-on plan proveriti prvi zahtev nakon duže
  neaktivnosti: bez Render wake stranice, bez client timeouta i sa prihvatljivim
  p95 vremenom za auth + prvi chat.
- [x] Podesiti quota/cost alarm za dnevni globalni AI limit.
- [ ] Napraviti pregled logova/alerta za:
  auth failure rate, AI timeout/fallback rate, moderation unavailable,
  Redis failure, HTTP 5xx i p95 total latency. `GET /metrics` je već živ i
  token-zaštićen na Renderu (bez `X-Metrics-Token` vraća 403, ne 404), pa spoljni
  watcher može da čita brojače bez parsiranja logova.


## 5. Build i SteamPipe

- [x] Napraviti **Windows Shipping** build iz UE 5.8 posle content locka.
  `v1.0.3` 03.09.; `v1.0.4` 05.09.; **`v1.0.5` skuvaren 07.09.**
  (`Builds/v1.0.5/Windows`) — DoorLock snap, pursuer bed na pogled / 5.5 s,
  flicker silent at 8 m. SteamPipe `setlive` `playtest`.
  `PackageWindowsShipping.bat` bez argumenta kuva u `Builds/Feature`.
  **`Builds/v1.0.0` se ne dira.** Live/default je još **v1.0.2**
  (`25008533`) dok se Set Live ne uradi svesno.

- [x] Proveriti da build ne sadrži:
  `steam_appid.txt`, pravi API ključ, game token, editor/debug sadržaj ili logove.
  Ranije provereno na starom `Builds/Alfa/Windows`; **v1.0.2** (`28.08.2026`)
  nema `steam_appid.txt` ni `.pdb`.
- [x] Ponovljeno na v1.0.2.
- [x] Ponoviti proveru tajni / `.pdb` / `steam_appid.txt` na **v1.0.5**
  (nema `steam_appid.txt` ni `.pdb` u `Builds/v1.0.5/Windows`).
- [ ] Pokrenuti Shipping EXE direktno na čistoj Windows mašini radi dependency
  provere.
- [x] Napraviti SteamPipe `app_build`/depot VDF i uploadovati Windows depot.
  Skripte su u `Tools/SteamPipe/`, depot `4982261`; sledeći upload je
  `Tools/SteamPipe/UploadPlaytest.bat`.
- [x] Postaviti build prvo na privatni `internal` ili `playtest` branch.
  **v1.0.5** ide na `playtest` (07.09.). Default/live je još v1.0.2
  `25008533`. Development `25008639` je na passwordovanom `valvereview`.
- [x] Instalirati build kroz Steam klijent, ne koristiti samo lokalni packaged
  folder. Shipping build pokrenut iz Explorera ne dobija Steam ticket, pa AI chat
  ne radi — QA se radi isključivo iz Library-ja.
- [ ] Posle QA postaviti odobreni build na default branch. Bezopasno je dok igra
  nije released, ali finalni build tamo treba da ide svesnom odlukom
  (SteamCMD `setlive` ne može `default`).


## 6. Release-candidate QA


### Kritični gameplay

- [x] Svih 9 loopova: advance/reset pravila, anomaly generation i tačan završetak.
- [x] Novi direktni elevator transition ne prihvata dupli input, ne ostavlja igrača zaključanog
  i teleportuje samo dok su vrata zatvorena/ekran skriven.

- [~] Svih 6 endinga i njihove mini-sekvence su dostižni; widget/terminal se
pojavljuje posle sekvence i Continue vraća u Main Menu. Prolaz je u principu
dobar; ostaje još jedan kontrolni prolaz po endingu i polish iz §1.

- [x] Item inspection, pursuer, Scale, Phantom i MaterialSwap anomaly smoke.
- [ ] Save migracija: stari save bez Clock anomalije ne kvari `ACH_SPOT_ALL`.
  (C++ već briše `ClockAnomaly` iz `SpottedAnomalies` pri startu.)

Polish smoke ponovljen i potvrđen na **v1.0.3** (04.09.2026.). Stari
v1.0.1 BuildID više nije QA cilj:

- [x] ~8/10 petlji ima anomaliju; loop 1 čist; Hide/Material češći, Pursuer ređi.
- [x] Nijedna aktivna anomalija nije nevidljiva (`AnomalyList` u konzoli).
- [x] Muzika sprata od ulaska u nivo, bez settings menija; posle Pursuer-a se vraća.
- [x] Flicker se čuje samo blizu tog svetla.
- [x] `F` pali/gasi lampu; ne radi u pauzi i inspekciji.
- [x] Zaključana vrata: zvuk + shake; open/close različiti.
- [x] Ending kuca, SKIP dopuni tekst, zatim Return to Main Menu.
- [x] sr/de/fr/ru: chat („Razmišlja…“), predugačka poruka, prompt u liftu,
  `TASK COMPLETE`, Help, SKIP.

### Dragojlo commitment (posle v1.0.3 / Valve QA)

- [x] Backend deploy sa `AI_COMMITMENT_ENABLED=true`.
- [x] **AI context tekstovi (07.09.).** Zone/kind stringovi na anomaly
  komponentama ažurirani. To **nije** isto što i observation volume.
- [x] **Editor migration:** nema starog `DragojloZoneVolume` (MCP 05.09.).
- [x] **9 `Loop9ObservationZoneVolume` u `FullOfficeMap`** (commit 07.09.):
  `OZ_OfficeDesks`, `OZ_BesideElevatorDoors`, `OZ_PottedPlantCabinets`,
  `OZ_MeetingRoom`, `OZ_BackRoomRadio`, `OZ_BackShelves`, `OZ_ToolShelf`,
  `OZ_StorageCorner`, `OZ_CorridorLiftsDesks`. Merge 05.09. (desk row +
  cabinets → office desks; copy alcove → back shelves). `v1.0.4` cook je
  posle postavljanja.
- [~] **Zone live QA (Andrija, v1.0.5).** Merni put je u kodu: `decoy_zone`
  samo iz tagged + registrovanog volume-a; `misdirect_location` naoružava
  target; ulazak posle saveta setuje `bVisitedSuggestedDecoy` i loguje
  `Dragojlo decoy zone visited`; sledeći chat šalje taj flag. Lift bez
  drugog poziva AI-u ne menja glas, ali flag i ending telemetry ostaju.
  100% „kako model priča o zoni“ nije statički garant — to se čuje samo
  u igri. Jedan ciljani run: traži da proveriš mesto → uđi u tu zonu →
  Output Log za visit → drugi poziv. Ako visit log postoji, sistem meri;
  ako AI i dalje zvuči isto, to je glas, ne rupa u overlap-u.
- [ ] `OZ_OfficeDesks` `ZoneId` = `near one of the office desks` (MCP često
  ne upisuje FName). Ostali ZoneId-ovi ručno editovani 05.09.
- [~] Box scale-ovi grubo oko tagged predmeta; lift cabin nije pokriven
  osim malog `OZ_BesideElevatorDoors`. Overlap još ručno.
- [~] Radio inspectable: `ObservationId=radio_set`. `ledger` / `family_photo`
  još čekaju ručni upis. Phantom/loop counter bez zone; Pursuer nije decoy.
- [ ] Sačuvaj mapu i Standalone proveri: ulazak pre saveta se ne računa; posle
  `misdirect_location` ulazak u baš sugerisanu zonu loguje
  `Dragojlo decoy zone visited`. Bez odgovarajućeg volume-a backend bezbedno
  pada na truthful savet umesto nemerljive lokacijske obmane.
- [ ] Standalone journal QA: uđi/izađi iz dve zone, pregledaj predmet, uspešno
  otvori/zatvori i probaj zaključana vrata, upali/ugasi lampu, pogledaj
  Pursuer-a i dopusti mu da stigne igrača; tek uspešan AI
  odgovor dodaje `call_completed`, a neuspešan odgovor ne. Sledeći sprat nema
  stare floor događaje, dok `run_summary` zadržava brojeve poziva/odluka.
- [ ] Odmah na prvom čistom spratu proveri da `floors_started=1` i da
  `seconds_on_floor` kreće približno od nule, a ne od vremena rada procesa.
- [ ] Proxy/backend log potvrđuje da je `observation_snapshot` ≤1024 UTF-8
  bajta, ima najviše 8 events/8 visited_zones i nema chat, koordinate, actor
  names, anomaly keys, commitment IDs ni relationship floatove.
- [x] Render flagovi usklađeni sa repo-om (08.09.): Render već vrti
  `AI_OBSERVATION_CONTEXT_ENABLED=true` (Andrija override, 07.09.), pa je i
  commitovani `.env` sad `true` da svež deploy bez Render ključa ne vrati
  tiho na `false`. Produkcioni set stoji u backend
  `docs/CONFIGURATION.md`. Render vrednost je uvek jača od `.env`.
- [ ] **Render: dodati `AI_COMMITMENT_WRONG_LIFT_CHANCE=0.5`** posle
  deploy-a backenda (08.09.). Bez ključa Symfony pada na `.env` = `0.5`, pa
  ništa ne puca — ali neka bude vidljiv da se posle launcha štima bez deploy-a.
- [x] Wrong-lift rebalans (backend, 08.09.): više ne traži misdirect +
  razotkrivenu kontradikciju. Uslovi: loop ≥7, aktivna ne-Pursuer anomalija,
  dependency ≥0.62, nije potrošen, i poluga — igrač je predao odluku
  (`pending_decision_surrender`) ili poslušao poslednji savet za lift
  (`followed_last_lift_advice`). Pun luk (lokacijska laž → optužba → predaja)
  laže uvek; svaki drugi kvalifikovani sprat je jedan stabilan roll po spratu
  (`AI_COMMITMENT_WRONG_LIFT_CHANCE`, default 0.5). Isti sprat se ne
  re-roll-uje drugim pitanjem. Testovi: `AdvicePolicyTest` (297 zelenih).
- [x] **Pursuer = mrtva linija (klijent, 08.09.).** Dok je Pursuer aktivan
  (ceo sprat, i posle despawna) `SayToAI` ne ide na backend: chat ispiše
  lokalizovani `ChatPursuerNoAnswer` („...veza se uspostavlja. Niko ne govori.
  Disanje koje čuješ ne dolazi iz slušalice.“), ne troši message slot, ne
  broji AI interakciju, ne menja Kindness. PO unosi dodati za svih 5 jezika
  → **treba GatherText pre cooka** da uđe u `Game.locres`; bez toga se vidi
  engleski izvor u svim jezicima (ne ruši ništa). **Nije u v1.0.5** (skuvan
  07.09.) — ide u v1.0.6 ako se kuva pre petka, inače prvi post-launch patch.
  Backend deo ne zavisi od cooka.
- [ ] Standalone QA Pursuer linije: `AnomalyPursuer` u konzoli → telefon →
  pošalji poruku → dead-line poruka, nema `Thinking…`, `MessagesSentThisLoop`
  ostaje 0, ending timeline ne broji poziv. Sledeći sprat chat radi normalno.
- [ ] Staging/live voice probe: accurate → misdirect → accusation → surrender →
  wrong-lift (`tools/dragojlo-voice-probe.php`, luna+terra, SR/EN/DE/FR/RU).
  Dodati i „surrender bez misdirecta“ na loop 7 sa chance=1.0 lokalno.
- [ ] Ručni QA: neutralan run bez laži; dependent run sa jednom pogrešnom
  lokacijom; Obedient kandidat sa najviše jednim pogrešnim liftom.
- [ ] Posle razotkrivene kontradikcije sledeći uspešan odgovor je jednom
  defanzivan (`advice.mode=confrontation`), ne priznaje namernu laž i ne daje
  novi lift/lokaciju; naredni odgovori se vraćaju na normalan policy.
- [ ] Ending telemetry log sadrži commitment agregate: offered/visited decoy,
  vreme do posete, exposed contradiction, lift advice/follow count i posebno
  da li je pogrešan lift poslušan. Nema koordinata ni chat teksta.
- [ ] Šest ending profila i dalje dostižni; Escape Together nije blokiran.
- [ ] Pratiti `ai.fallback`, format greške i raspodelu endinga posle uključivanja.


### Steam i online

- [x] Steam ticket → backend session → prvi chat zahtev radi na App ID `4982260`.
- [x] **Audit rate limiting / AI troška (08.09.).** Redosled na `/api/chat`:
  HMAC session token (legacy game token odbijen u kodu u prod) → burst 20/min
  po IP → validacija tela → IP 300/dan → igrač 120/dan → igrač 2000/mesec →
  globalno 5000/dan → tek onda moderacija + AI. Brojači su u Redis-u (deljeni,
  preživljavaju deploy); `/readyz` na live-u vraća `ready`, forged/legacy
  token vraćaju 403 bez troška. Bez Steam naloga koji poseduje igru napadač ne
  može da potroši ništa; sa jednom kopijom max 120/dan. Poruka spuštena sa
  4000 na 1000 znakova (backend + klijent u `main`). Detalji i preostale meke
  tačke: backend `docs/SECURITY_AND_PRIVACY.md`. Ostaje ručno: spend limit na
  provider nalogu (gore).
- [x] Klijent traži svež Steam Web API ticket asinhrono (`WebAPI:Loop9`), a backend
  ga proverava preko publisher API-ja sa istim identity parametrom.
- [x] Steam Overlay i achievement toast rade.
- [ ] Offline start i gubitak mreže tokom chata ne ruše igru i refundiraju pokušaj.
- [x] AI input/output moderation: bezbedan tekst prolazi; blokiran i unavailable
  slučaj daju in-fiction fallback.
- [x] Cold-start/timeout: `Thinking…` i `Still thinking…` rade; zahtev završi odgovorom
  ili lokalizovanom greškom pre client timeouta od 65 s.
- [~] Jedan instant EN fragment na playtestu (~„… shift …“, 07.09.). Nije
  connection-lost / LOW SIGNAL / moderation fallback — ti su cele, lokalizovane
  rečenice. Verovatnije kratak model stub ili low-stability mid-thought
  (prompt ima „this shift to end“). Ako se ponovi: zapiši petlju i da li je
  stiglo odmah posle slanja. Nije launch bloker.
- [ ] Telemetry `run-finished` stiže samo sa validnom sesijom.
- [ ] Steam Cloud: odigraj → izađi → druga mašina/obrisan lokalni save →
  `SeenEndings` / `SpottedAnomalies` se vrate. Ovo je i persist QA iz §3. Od
  25.08. arhiva se ionako dopunjava iz Steama, pa Cloud više nije jedini put;
  ako ovaj test padne, nije bloker koliko je bio.


### Platforma, UI i performanse

- [x] EN/SR/DE/FR/RU: meni, settings, chat, promptovi, ending i terminal.
  Ponoviti posle GatherText-a od 25.08 zbog Help/SKIP/novih stringova.
- [ ] Tastatura/miš i gamepad kompletan prolaz; floating keyboard na Deck-u ili
  Steam Input testu.
- [x] Alt-Tab, promena rezolucije/fullscreena, pause/resume i Steam Overlay.
- [x] 30 min soak bez memory growtha, stale timera/delegata ili duplih widgeta.
- [~] Unreal Insights + `stat unit` / `stat game` / `stat gpu` nisu vođeni
  kao sesija. Informalni smoke 07.09. na kućnom Windowsu (Ryzen 7 5800X,
  16 GB DDR4-3200, RTX 5060 Ti 16 GB): **~80 FPS Epic**, Low znatno više.
  To nije min-spec (GTX 1060 / 8 GB) ni Deck.
- [~] VT anomaly teksture / peak VRAM nisu mereni Insights-om. 16 GB VRAM
  na gornjoj mašini nije dokaz za 6–8 GB target. Nije launch bloker ako
  Epic/Low izgledaju stabilno na `v1.0.3` cooku.
- [ ] Test na čistoj mašini bez Unreal Engine-a i lokalnih config fajlova.


## 7. Valve review i Coming Soon

- [x] Publishovati sve Store Presence promene (uključujući trailer).
- [x] Store Page poslata na Valve review ~22.08.2026.; **odobrena**.
  Javna Coming Soon stranica:
  https://store.steampowered.com/app/4982260/Loop_9/ (`?beta=0`, 27.08.2026.).
- [x] Poslati release-candidate **build** na Valve review. Shipping
  v1.0.2 `25008533` + `valvereview` Development `25008639`.
- [x] Valve build review **prošao 03.09.2026.** Steamworks spreman za
  release. `valvereview` lozinka promenjena 07.09.
- [x] Objaviti Coming Soon stranicu najmanje **14 dana** pre release-a.
  Objavljena **27.08.2026.** Najraniji Valve datum: **10.09.2026.**
  Planirani release: **petak 11.09.2026.**
- [x] Steam Direct fee je plaćen 15.07.2026. Obavezni 30-dnevni Direct period
  istekao je približno **14.08.2026.** To nije release datum; čeka se 14 dana
  od Coming Soon + RC build review + QA.


## 8. Release day

- [ ] Zamrznuti kod i sačuvati tačan commit/build ID koji ide live.
- [ ] Potvrditi `/readyz`, Redis, AI provajdere, quota alarm i Render kapacitet.
- [ ] Završiti Steam release proces i postaviti odobreni build live.
- [ ] Instalirati javni build sa drugog Steam naloga i uraditi 15-min smoke.
- [ ] Pratiti auth, chat, moderation, latency i 5xx logove tokom prvih sati.
- [ ] Imati prethodni stabilni depot/build spreman za rollback.


## 9. Post-release (nije launch bloker)

Radi se posle Coming Soon / live-a, samo ako ostane vreme. Ne blokira Valve review.

- [~] Ending session timeline: C++ sada sam crta redove ispod naslova/opisa
(`UEndingWidget::PopulateTimeline` ← `BuildRunEventCards()`), na svih šest
`WBP_Ending_*` bez Event Graph-a. Linija `Resets | AI interactions` je
sakrivena. Ikonice: `Content/MyStuff/UI/Timeline/ui_icon_*.png` (editor treba
da ih uveze). Ending tekst ostaje na ekranu; Archive je iznad Quit.
Ostaje vizuelni QA: da timeline ne prekrije Continue i da TWO CALLS / boje
prstenova sede. Bez grafa sirovih relationship brojeva.
- [~] Main-menu dosije / arhiva smena: C++ `ShiftArchiveWidget` čita `SeenEndings`.
`WBP_MainMenu` nema dugme `Archive`, pa ga `UMainMenuWidget` sada klonira
iz `Settings` (isto `WBP_Button`) i veže na `OnArchiveClicked`. Opciono
`ui_archive_star_plate` kao pozadina i dalje nije urađena. Animaciju ne raditi.
Brief:
`[docs/HOME_EDITOR_TIMELINE_AND_ARCHIVE.md](docs/HOME_EDITOR_TIMELINE_AND_ARCHIVE.md)`.

- [x] AI kontekst anomalija: C++ i backend su gotovi, a `FullOfficeMap` je sada
  označen. Od 35 anomaly komponenti 34 imaju `AnomalyObjectKind`;
  `PhantomMessage` po pravilu ima praznu zonu, a `PursuerAnomaly` je namerno
  ostavljen neoznačen jer nema fiksno mesto (netagovane komponente se
  preskaču, ne blokiraju). Nijedan string ne prelazi 48 znakova i nijedan ne
  koristi ime aktera. Upotrebljene zone (05.09. merge): near the office desks,
  cabinets with potted plants, meeting room, back room with the radio, back
  shelves (includes former copy alcove), tool shelf, storage corner, corridor
  between lifts and desks, beside the elevator doors.
  Pravila i primeri: `[docs/ANOMALIES.md](docs/ANOMALIES.md#ai-context-tagging)`.
- [x] **Ručni prolaz AI konteksta (Andrija, 07.09.).** `AnomalyZone` /
  `AnomalyObjectKind` tekstovi ažurirani predmet po predmet na
  `FullOfficeMap`. Phantom bez zone; Pursuer sme prazan. Veći anomaly
  beatovi (blackout / slam) **ne radimo za sada**.

- [ ] **Lore u igri — inspect papiri.** Nije Valve / 11.09 bloker, i
  **ne sada.** Andrija piše tekstove kad dođe do toga. Smisao ostaje:
  listovi stoje od loop 1, document inspect (levo papir, desno tekst),
  bez `AnomalyComponent`. Vidi `[docs/LORE.md](docs/LORE.md)` §4.

- [ ] **Steam Deck kompatibilnost.** Dodato 28.08.2026. kao post-release stavka.
  Deck status (`Verified` / `Playable` / `Unsupported`) testira Valve, ne ti; ti
  ga samo pošalješ na review iz Steamworks-a. Nije launch bloker, ali stoji
  vidljivo na store stranici pa vredi.
  - Realno najveći rizik za ovu igru je **čitljivost teksta**, ne performanse.
    Deck je 7" ekran na 1280×800, a Loop 9 je tekstualna igra — terminal, chat,
    ending ekrani, krediti. Fontovi dimenzionisani za 1080p+ tamo postaju
    sitni. To je najčešći razlog za `Playable` umesto `Verified`.
  - **Unos teksta.** Chat sa Dragojlom traži kucanje, a Deck to rešava Steam
    floating keyboard-om. Već stoji kao stavka u §6 („Platforma, UI i
    performanse"), i direktno je Verified kriterijum.
  - **Grafika.** `DefaultEngine.ini` nosi Lumen, ray tracing i virtual textures
    (`r.RayTracing=True`, `r.DynamicGlobalIlluminationMethod=1`). Deck APU to ne
    nosi na 15 W. Treba Deck preset preko postojećeg `PreferredGraphicsQuality`
    (0–3) u `Loop9GameSettingsSubsystem` — kuka već postoji, ne treba nova.
  - **Cloud.** Deck vrti igru kroz Proton, pa `WinAppDataLocal` root pokazuje u
    Proton prefiks. `gameinstall` je jedini root validan na svim platformama —
    isti razlog zbog kog je predložen u §2 za Cloud problem. Ako se tamo pređe
    na `gameinstall`, ovo je usput rešeno.

- [x] Help slike — ne radimo za sada (26.08.2026.).
- [x] Replacement terminal: typewriter zvuk + SKIP (26.08.2026.).
  Phone pickup, ring, chat mumble i koraci imaju C++ default (26.08.).
  Prompt-complete i chat typing i dalje prazni; nisu launch bloker.
- [x] Credits ekran odvojen od Help-a + How to Play label (26.08.2026.).


---

## Trenutni kritični put

Valve build review **prošao 03.09.2026.** `valvereview` lozinka promenjena
07.09. Feature freeze. Kandidat **v1.0.5** (vrata / pursuer / flicker).
Live/default je još v1.0.2 **`25008533`**. `Builds/v1.0.0` fallback nije
diran. Valve dozvoljava od 10.09.; store datum: **petak 11.09.2026.**

1. Steam playtest v1.0.5: ending runovi, journal, toast. Jedan zone run
   (§6) ako stigne. Novi cook samo ako QA nađe blocker.
2. Petak 11.09.: Set Live na default + release day §8. Ne Set Live pre toga.
3. Observation volume-i i AI context tagovi su u mapi.
4. **08.09. u `main`, nije u v1.0.5:** Pursuer mrtva linija (klijent, treba
   GatherText + cook) i wrong-lift rebalans + `AI_COMMITMENT_WRONG_LIFT_CHANCE`
   (backend, deploy nezavisan od cooka; radi i sa v1.0.2/v1.0.5 klijentom jer
   koristi flagove koje klijent već šalje). Odluka: v1.0.6 pre petka ili prvi
   patch posle launcha. Ako se kuva v1.0.6, u Render dodati i
   `AI_COMMITMENT_WRONG_LIFT_CHANCE=0.5`.
5. Posle launcha: lore papiri (`docs/LORE.md` §4), Deck, boot logo; dve nedelje
   `run.ended.*` brojki pre bilo kakvog pomeranja ending pragova.
