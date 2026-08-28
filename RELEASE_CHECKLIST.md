# Loop 9 — authoritative release checklist

Poslednje ažuriranje: **28.08.2026.**
Steam App ID: **4982260**

> **Valve je oborio build `24910264` (28.08.2026.).** Tri stavke, plan i tekst
> odgovora su u
> `[Marketing/Steam/VALVE_REVIEW_REPLY.md](Marketing/Steam/VALVE_REVIEW_REPLY.md)`:
> nedostaju common redistributables, Cloud ne sinhronizuje, i nisu mogli da
> verifikuju šest endinga. Prva dva su Steamworks konfiguracija bez rebuilda;
> treće traži Development build na passworded grani. Uz to je Valve blokirao
> prodaju u Kini — informacija, ne zadatak.
>
> **Urađeno 28.08.:** redistributables čekirani i publish-ovani; cela Cloud
> strana pregledana i sve što se vidi iz Steamworksa je ispravno
> (developers-only **nije** bio čekiran, kvota i Auto-Cloud pravilo su tačni).
>
> **Ostalo, sve traži Windows mašinu:**
> 1. Naći zašto Cloud ne sinhronizuje. Pustiti `testappcloudpaths 4982260` u
>    Steam konzoli i potvrditi da postoji
>    `%LOCALAPPDATA%\Loop9\Saved\Config\Windows\Game.ini`. Ako je fajl pored
>    `.exe`, prebaciti Root na `gameinstall`.
> 2. `git pull` + rebuild (da uđu on-screen poruke debug komandi), pa
>    `Tools\PackageWindowsDevelopment.bat Debug`.
> 3. Napraviti passworded granu `valvereview`, privremeno prebaciti
>    `contentroot` na `Builds/Debug/Windows`, uploadovati, vratiti na
>    `Builds/v1.0.1/Windows`, Set Live na tu granu.
> 4. Odgovoriti na tiket tekstom iz `VALVE_REVIEW_REPLY.md` §4 i markirati
>    **v1.0.1 (`24980937`)** za review, ne stari oboreni build.
>
> Shipping ne treba ponovo kuvati: sve izmene od 28.08. su pod
> `#if !UE_BUILD_SHIPPING`, pa je v1.0.1 funkcionalno nepromenjen.

Ovo je jedini dokument koji prati spremnost za release. Tehničke tabele ostaju u
`[STEAM_ACHIEVEMENTS.md](STEAM_ACHIEVEMENTS.md)`, marketinški tekst u
`[Marketing/Steam/STORE_PAGE.md](Marketing/Steam/STORE_PAGE.md)`, a cinematic
dokumentacija u `[docs/CINEMATICS_AND_AUDIO.md](docs/CINEMATICS_AND_AUDIO.md)`.
Zvukovi za dodelu: `[docs/AUDIO_ASSIGNMENT_CHECKLIST.md](docs/AUDIO_ASSIGNMENT_CHECKLIST.md)`.
Otvoreni editorski ostaci polish prolaza:
`[docs/POLISH_PASS_EDITOR_TASKS.md](docs/POLISH_PASS_EDITOR_TASKS.md)`.

Legenda: `[ ]` nije gotovo · `[~]` podešeno, ali nije završno verifikovano ·
`[x]` završeno i potvrđeno

**Važno o dva stanja.** `[x]` ispod znači da je stvar u `main` repou, ne
nužno i na Steam playtestu. **`Builds/v1.0.0`** je trenutni fallback Shipping
drop — ne overwrite-ovati. WIP cook (novi featurei / bugfix) ide u
**`Builds/Feature`**. Nova verzija (`v1.0.1` itd.) dobija **novi folder** i tamo
se kuva. SteamPipe contentroot je **`Builds/v1.0.1/Windows`**. **`Builds/v1.0.0`
ostaje fallback** i ne overwrite-uje se. Stari `Builds/Alfa` i `Builds/Beta`
više nisu cilj. Live playtest na Steamu je **v1.0.1** (BuildID `24980937`,
28.08.2026). Prethodni playtest cook od 24.08 je bio BuildID `24910264`.
Prethodni Alfa playtest je bio BuildID `24782464`.
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

- [~] U `BP_LoopElevatorTransitionDirector` postavljeni `Button Press Sound`
(`Sound/Elevator/ElevatorButtonPress`) i looping `Travel Sound`
(`Sound/Elevator/ElevatorTravelLoop`, `Looping` = true na SoundWave-u) — i na
class defaults i na instancu u `FullOfficeMap`. Zvukovi su uvezeni iz CC0
izvora; licence i izvorni URL-ovi su u
`[docs/AUDIO_ASSIGNMENT_CHECKLIST.md](docs/AUDIO_ASSIGNMENT_CHECKLIST.md)`
(atribucija nije potrebna). Volume/fade ostaju na defaultima
(`1.0 / 1.0 / 0.15 s`) — **ostaje jedno slušanje u igri** da se potvrdi mix
i da travel loop ne slaže loopove kroz ponovljene vožnje.

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

Pragovi su podešeni za prvi prolaz od približno 9–17 odluka, bez znanja skrivenih
ključnih reči. Potvrda prirodnih profila ostaje za kasnije (26.08.2026.).

- [ ] **Paranoid Survivor:** 0–2 AI razgovora ili eksplicitno visok suspicion /
  nizak trust. Ovo je namerni ending za igrača koji ignoriše Dragojla.
- [ ] **Escape Together:** oko 4–7 normalnih razgovora, većina tačnih odluka,
  osnovna pristojnost i bez preterane zavisnosti; očekivani pozitivan first-run.
- [ ] **Cold Betrayal:** najmanje 5 razgovora, solidna saradnja, ali približno dve
  jasno neprijatne/uvredljive poruke.
- [ ] **Obedient Fool:** najmanje 8 razgovora i oko 5 predaja odluke
  (`DEPENDENCY=1` od AI-ja), uz nizak suspicion.
- [ ] **Merged Memory:** najmanje 6 razgovora, kindness/cooperation ostaju
  pozitivni, ali duži run sa dovoljno grešaka spusti AI stability na oko 0.72.
- [ ] **The Replacement:** redak, ali realno dostižan profil: najmanje 11
  razgovora, visoki kindness/cooperation/dependency i duži nestabilan run sa
  AI stability oko 0.70 ili niže.
- [ ] QA: napraviti po jedan kontrolisan run za svih šest profila i proveriti da
  evaluator ne vraća `Paranoid Survivor` kao slučajni fallback za pozitivan run.
- [ ] QA dependency: na bilo kom jeziku, predaja odluke Dragojlu diže
  `[STATE]DEPENDENCY=1` i gura ka Obedient Fool; samostalna odluka spušta.


### Runtime hardening (urađeno pre 21.08)

- [ ] Rebuildovati `Loop9Editor` posle C++ izmena od 22–25.08 (Help, lampa,
  vrata, typewriter, muzika sprata, dead-code cleanup). Stariji rebuild od
  pre 21.08 više nije dovoljan.
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
- [x] Item inspection smoke:
  otvaranje/zatvaranje, rotacija 15 s, bez ljubičastih artefakata, povratak inputa
  i pause menija.
- [x] MaterialSwap smoke za Magazine, I01, F01 i D01; potvrđeno vraćanje originalnog
  materijala na sledećem loopu.


### Polish pass u `main` (22–25.08.2026.) — nije na trenutnom Steam playtestu

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
- [ ] **Raspored dugmadi (28.08.):** podići vertikalni stack u main menuju i
  pomeriti `Return to Main Menu` udesno na ending ekranima — sada je praktično
  na sredini, i treba u **oba** widgeta (`EndingWidget` i
  `ReplacementTerminalWidget`). Oba su WBP layout, ne C++; detalji i zamke u
  `[docs/POLISH_PASS_EDITOR_TASKS.md](docs/POLISH_PASS_EDITOR_TASKS.md)` §2b.


## 2. Steamworks — Store Presence

- [x] App kreiran; App ID je `4982260`.
- [x] Basic Info, platforma, jezici, žanrovi, features i launch option
  (`Loop9.exe`) popunjeni.
- [~] **Installation → Common Redistributables:** `DirectX End-User Runtimes
  (June 2010)` i `Microsoft Visual C++ Redistributable 2022` čekirani i
  publish-ovani **28.08.2026.** Bez toga UE prereq installer iskače kao
  third-party launcher i Valve obara build (`24910264`). Nije tražilo rebuild.
  Potvrda stiže tek kad Valve ponovi review.
- [ ] Passworded grana `valvereview` sa Development buildom
  (`Tools/PackageWindowsDevelopment.bat`) da recenzent može do šest endinga.
  Skinuti je posle odobrenja.
- [x] Content Survey popunjen sa runtime AI i AI-assisted marketing disclosure.
- [x] EN/SR store opis pripremljen.

- [x] DE/FR/RU lokalizovani opisi su uneti i sačuvani u Steamworksu; izvor je
`Marketing/Steam/STORE_PAGE.md`.

- [x] Cena `$4.99` i Valve regional pricing poslati.
- [x] Sačekati potvrdu pricing promena.
- [~] Steam Cloud Auto-Cloud podešen za
  `WinAppDataLocal/Loop9/Saved/Config/Windows/Game.ini`.
  Tu žive viđeni endinzi i uočene anomalije (`SeenEndings`, `SpottedAnomalies`).
  **Valve javlja 28.08. da sync ne radi i da Properties → General ne pokazuje
  ništa.** Kvota (`10485760` B / `10` fajlova) i Auto-Cloud pravilo su
  provereni 28.08. i ispravni su; preview razrešava tačnu putanju.
- [x] **„Enable cloud support for developers only"** (Cloud → Beta Testing)
  provereno 28.08. — **nije bilo čekirano**, pa nije uzrok. Bio je glavni
  osumnjičeni jer taj čekboks gasi Auto-Cloud i sakriva ikonicu.
- [ ] **Ostaje jedini sumnjivac: da li `Game.ini` postoji tamo gde pravilo
  gleda.** Sve što se vidi iz Steamworksa je ispravno, pa se uzrok mora tražiti
  na disku. Na Windowsu pustiti `testappcloudpaths 4982260` u Steam konzoli
  (`steam://open/console`) — ispisuje koje fajlove pravilo stvarno hvata. Ako je
  lista prazna, prebaciti Root sa `WinAppDataLocal` na `gameinstall`. Detalji u
  `[Marketing/Steam/VALVE_REVIEW_REPLY.md](Marketing/Steam/VALVE_REVIEW_REPLY.md)`
  §2.5.
- [x] Odlučeno da `GameUserSettings.ini` **ne** ide na Auto-Cloud. Steamov
  best-practice traži da se izbegne machine-specific config, a taj fajl je
  rezolucija i quality scalability. `Game.ini` je pravi save i postoji od prvog
  pokretanja, pa Properties → General ima šta da pokaže i bez njega.
- [ ] **Cloud se verifikuje samo na Shipping buildu.** Development build drži
  Saved pored `.exe` umesto u `%LOCALAPPDATA%`, pa debug grana iz §3 nije
  merodavna i to treba reći recenzentu.
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
- [x] Shortcut ICO i App Icon JPG imaju spremne minimalističke v2 varijante.
- [x] Dodati opcioni Page Background `1438×810`.
- [x] Gameplay trailer snimljen, montiran i uploadovan u Steamworks
  (Store Presence → Trailers + Publish). Valve store review je **odobren**;
  Coming Soon stranica je javna od **27.08.2026.** Thumbnail je u
  `Marketing/Steam/StoreUpload/trailer_thumbnail_1920x1080.jpg`.
- [ ] Creator Homepage može posle Coming Soon stranice; nije release bloker.


### Branding u igri — ikonice, splash i boot sekvenca

Steamworks grafika je gotova, ali **sam build još nosi Unreal Engine identitet.**
Projekat uopšte nema `Build/` folder, pa se paketovana igra kuva sa default
Epic ikonicom i default splash-om. Ovo su editorski / fajl zadaci na Windowsu.

- [ ] **Ikonica `.exe`-a.** Napraviti `Build/Windows/Application.ico` (multi-size
  ICO: 256, 128, 64, 48, 32, 16 px). To je ikonica koju vidiš u taskbaru, u
  Alt-Tabu i na shortcut-u paketovane igre. Postojeći „Shortcut ICO v2" iz
  `Marketing/` je dobar izvor. Mora da postoji **pre** cooka.
- [ ] **Splash pri pokretanju.** `Content/Splash/Splash.bmp` za igru
  (`EdSplash.bmp` za editor, opciono). Bez toga se vidi Epicov default.
- [ ] Napomena: ako ikonicu Unreal Engine-a vidiš dok si **u editoru**, to je
  normalno i ne može se promeniti — to je `UnrealEditor.exe`, ne tvoja igra.
  Proveri na paketovanom buildu pre nego što se juriš za bugom.
- [ ] **Boot logo sekvenca** (studio žig pre menija). Odluka i plan; prvo treba
  ime studija, jer je Developer / Publisher u `STORE_PAGE.md` još uvek
  „Andrija Stanišić (ili ime studija ako ga registruješ)". Preporuka je UMG
  animacija u postojećem terminal / typewriter jeziku igre, 2–3 sekunde,
  skip na bilo koji input, ne startup `.mp4`.
- [ ] **Ne dodavati Unreal Engine logo u boot sekvencu.** Po Epicu, prikaz UE
  logotipa traži posebnu dozvolu preko branding zahteva; AAA igre koje ga
  prikazuju imaju custom licencu. EULA zahteva samo tekst u kreditima.
- [x] **UE atribucija u kreditima (obavezna po EULA).** Dodata `ENGINE` sekcija
  u `CreditsWidget` 28.08.2026. sa tačnom formulacijom koju Epic traži.
  Namerno nije lokalizovana da prevod ne bi izmenio pravni tekst.
- [ ] GatherText posle ovoga da `ENGINE` naslov uđe u `.locres` (telo sekcije je
  `FText::FromString` i ne gather-uje se, što je namerno).


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
- [ ] Verifikovati popravku na Windows buildu: log treba da ima
  `Achievements: unlock ACH_FIRST_CALL -> OK` i toast treba da se pojavi.
  Obavezno iz Steam Library-ja — iz Explorera nema Steam sesije.

- [x] **Arhiva se sama popravlja (25.08.2026.):** `GetSeenEndingIds` i
  `RecordSpottedAnomalies` više ne čitaju samo `Game.ini`, nego ga spajaju sa
  onim što Steam drži za šest `ACH_ENDING_*` i devet `ACH_SPOT_*`. Isti podatak,
  dva izvora: Steam preživi izgubljen ili Cloudom pregažen fajl, a fajl radi
  offline i bez Steama, gde Steam ne vraća ništa. Promašaj sa Steama se nikad ne
  čita kao „nije otključano“, pa nema lažnog brisanja. Spojena lista se upiše
  natrag samo kad se dve razlikuju. Time Cloud prestaje da bude nosiva greda za
  arhivu i za `ACH_SPOT_ALL`.
- [ ] Provera: pusti jedan ending, obriši `SeenEndings` red iz `Game.ini`,
  otvori Archive. Čvor treba da se vrati sa Steama i red da se ponovo upiše.

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

- [~] Napraviti **Windows Shipping** build iz UE 5.8 posle content locka.
  `Tools/PackageWindowsShipping.bat` bez argumenta kuva u `Builds/Feature`.
  `PackageWindowsShipping.bat v1.0.1` kuva u `Builds/v1.0.1`. **`Builds/v1.0.0`
  se ne dira** dok se eksplicitno ne zatraži. Steam playtest je još cook od
  24.08.2026. Feature/sledeći versioned cook treba da uključi: popravku
  achievementa, muziku sprata, kucanje na endinzima, uvezene zvuke vrata i
  typewriter, i nov `.locres`.

- [x] Proveriti da build ne sadrži:
  `steam_appid.txt`, pravi API ključ, game token, editor/debug sadržaj ili logove.
  Ranije provereno na starom `Builds/Alfa/Windows`; **v1.0.1** (`28.08.2026`)
  nema `steam_appid.txt` ni `.pdb`.
- [x] Ponoviti tu proveru na sledećem versioned cooku (`v1.0.1`).
- [ ] Pokrenuti Shipping EXE direktno na čistoj Windows mašini radi dependency
  provere.
- [x] Napraviti SteamPipe `app_build`/depot VDF i uploadovati Windows depot.
  Skripte su u `Tools/SteamPipe/`, depot `4982261`; sledeći upload je
  `Tools/SteamPipe/UploadPlaytest.bat`.
- [x] Postaviti build prvo na privatni `internal` ili `playtest` branch.
  v1.0.1 je live na passwordovanom `playtest` (BuildID `24980937`, 28.08.2026).
  Prethodni live BuildID `24910264` je bio cook od 24.08.
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

Polish smoke na **novom** `v1.0.0` Shipping cooku (Steam playtest od 24.08 ovo ne pokriva):

- [ ] ~8/10 petlji ima anomaliju; loop 1 čist; Hide/Material češći, Pursuer ređi.
- [ ] Nijedna aktivna anomalija nije nevidljiva (`AnomalyList` u konzoli).
- [ ] Muzika sprata od ulaska u nivo, bez settings menija; posle Pursuer-a se vraća.
- [ ] Flicker se čuje samo blizu tog svetla.
- [ ] `F` pali/gasi lampu; ne radi u pauzi i inspekciji.
- [ ] Zaključana vrata: zvuk + shake; open/close različiti.
- [ ] Ending kuca, SKIP dopuni tekst, zatim Return to Main Menu.
- [ ] sr/de/fr/ru: chat („Razmišlja…“), predugačka poruka, prompt u liftu,
  `TASK COMPLETE`, Help, SKIP.


### Steam i online

- [x] Steam ticket → backend session → prvi chat zahtev radi na App ID `4982260`.
- [x] Klijent traži svež Steam Web API ticket asinhrono (`WebAPI:Loop9`), a backend
  ga proverava preko publisher API-ja sa istim identity parametrom.
- [x] Steam Overlay i achievement toast rade.
- [ ] Offline start i gubitak mreže tokom chata ne ruše igru i refundiraju pokušaj.
- [x] AI input/output moderation: bezbedan tekst prolazi; blokiran i unavailable
  slučaj daju in-fiction fallback.
- [x] Cold-start/timeout: `Thinking…` i `Still thinking…` rade; zahtev završi odgovorom
  ili lokalizovanom greškom pre client timeouta od 65 s.
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
- [ ] Unreal Insights + `stat unit`, `stat game`, `stat gpu` na minimalnoj i
  preporučenoj konfiguraciji.
- [ ] Proveriti nove VT anomaly teksture u cooked buildu i peak VRAM; izvorni novi
  `.uasset` fajlovi trenutno zauzimaju oko 158 MB u repou.
- [ ] Test na čistoj mašini bez Unreal Engine-a i lokalnih config fajlova.


## 7. Valve review i Coming Soon

- [x] Publishovati sve Store Presence promene (uključujući trailer).
- [x] Store Page poslata na Valve review ~22.08.2026.; **odobrena**.
  Javna Coming Soon stranica:
  https://store.steampowered.com/app/4982260/Loop_9/ (`?beta=0`, 27.08.2026.).
- [ ] Poslati release-candidate **build** na Valve review.
- [ ] Ispraviti eventualne review primedbe i ponovo poslati.
- [x] Objaviti Coming Soon stranicu najmanje **14 dana** pre release-a.
  Objavljena **27.08.2026.** Najraniji release datum: **10.09.2026.**
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
  koristi ime aktera. Upotrebljene zone: desk row, overhead cabinets, cabinets
  with potted plants, filing cabinets, meeting room, back room with the radio,
  tool shelf, storage corner, corridor between lifts and desks.
  Pravila i primeri: `[docs/ANOMALIES.md](docs/ANOMALIES.md#ai-context-tagging)`.

- [x] Help slike — ne radimo za sada (26.08.2026.).
- [x] Replacement terminal: typewriter zvuk + SKIP (26.08.2026.).
  Phone pickup, ring, chat mumble i koraci imaju C++ default (26.08.).
  Prompt-complete i chat typing i dalje prazni; nisu launch bloker.
- [x] Credits ekran odvojen od Help-a + How to Play label (26.08.2026.).


---

## Trenutni kritični put

1. Zatvori Unreal Editor, pa `Tools/PackageWindowsShipping.bat` (WIP izlaz
   `Builds/Feature`; nova verzija npr. `PackageWindowsShipping.bat v1.0.1`).
   **`Builds/v1.0.0` se ne overwrite-uje.** Typewriter + vrata `.uasset` i
   GatherText su urađeni; bez zatvorenog editora cooker udara na MCP port.
2. Lokalni QA Feature cooka (mix, vrata, ending i terminal kucanje, jezici,
   How to Play / Credits, alt muzika, pursuer bed). Achievementi i AI chat
   samo iz Steam Library-ja posle uploada.
3. Achievement test po grupi + persist QA iz §3 + ending balance (šest profila).
4. `UploadPlaytest.bat` → Steamworks Set Live na `playtest` (i `default` kad
   odlučiš). SteamCMD `setlive` ne može `default`.
5. Na tom Steam buildu: Cloud, offline, gamepad, polish smoke iz §6.
6. Store je javan (Coming Soon od 27.08.). Najraniji release **10.09.2026.**
   Sledeći Valve korak: RC **build** na review, ne store page.
