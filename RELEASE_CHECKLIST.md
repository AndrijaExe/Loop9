# Loop 9 — authoritative release checklist

Poslednje ažuriranje: **21.08.2026.**
Steam App ID: **4982260**

Ovo je jedini dokument koji prati spremnost za release. Tehničke tabele ostaju u
`[STEAM_ACHIEVEMENTS.md](STEAM_ACHIEVEMENTS.md)`, marketinški tekst u
`[Marketing/Steam/STORE_PAGE.md](Marketing/Steam/STORE_PAGE.md)`, a cinematic
dokumentacija u `[docs/CINEMATICS_AND_AUDIO.md](docs/CINEMATICS_AND_AUDIO.md)`.
Zvukovi za dodelu: `[docs/AUDIO_ASSIGNMENT_CHECKLIST.md](docs/AUDIO_ASSIGNMENT_CHECKLIST.md)`.

Legenda: `[ ]` nije gotovo · `[~]` podešeno, ali nije završno verifikovano ·
`[x]` završeno i potvrđeno

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
- [ ] U `BP_LoopElevatorTransitionDirector` postaviti `Button Press Sound` i
  looping `Travel Sound`, zatim podesiti njihove volume/fade vrednosti.
- [ ] Posle C++ rebuilda otvoriti i resaveovati `FullOfficeMap`/director Blueprint,
  pa obrisati sada nepotrebne `LS_Elevator_Lit` i `LS_Elevator_Dark` assete tek
  kada Reference Viewer potvrdi da više nemaju reference.
- [x] Implementiran C++ ending Sequence player sa šest soft-reference slotova,
  watchdogom i postojećim fade/widget fallbackom.
- [x] Napravljeno i povezano šest osnovnih 3–8 s ending Level Sequence asseta.
- [ ] Vizuelno i zvučno dotegnuti svih šest ending mini-sekvenci: kamera,
  svetlo, sitne prop animacije i završni prelaz u postojeći widget/terminal.
  Detaljan Unreal MCP handoff:
  `[docs/UNREAL_MCP_ENDING_SCENES_HANDOFF.md](docs/UNREAL_MCP_ENDING_SCENES_HANDOFF.md)`.



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
ključnih reči. Pre finalnog builda potvrditi ove prirodne profile:

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

- [ ] Rebuildovati `Loop9Editor` posle trenutnih C++ anomaly/debug popravki.
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
- [x] Ponovo pokrenuti GatherText jer su `de/fr/ru/sr` PO fajlovi menjani posle
  poslednjih commitovanih `.locres` fajlova; zatim smoke-testirati svih pet jezika.
- [x] Item inspection smoke:
  otvaranje/zatvaranje, rotacija 15 s, bez ljubičastih artefakata, povratak inputa
  i pause menija.
- [x] MaterialSwap smoke za Magazine, I01, F01 i D01; potvrditi vraćanje originalnog
  materijala na sledećem loopu.



## 2. Steamworks — Store Presence

- [x] App kreiran; App ID je `4982260`.
- [x] Basic Info, platforma, jezici, žanrovi, features i launch option
  (`Loop9.exe`) popunjeni.
- [x] Content Survey popunjen sa runtime AI i AI-assisted marketing disclosure.
- [x] EN/SR store opis pripremljen.

- [x] DE/FR/RU lokalizovani opisi su uneti i sačuvani u Steamworksu; izvor je
`Marketing/Steam/STORE_PAGE.md`.

- [x] Cena `$4.99` i Valve regional pricing poslati.
- [x] Sačekati potvrdu pricing promena.
- [x] Steam Cloud Auto-Cloud podešen za
  `WinAppDataLocal/Loop9/Saved/Config/Windows/Game.ini`.
- [ ] Dodati `GameUserSettings.ini` samo ako želiš sync grafike i jezika između
  računara; nije release bloker.
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
- [ ] Snimiti i montirati gameplay trailer. Trailer je veoma preporučen pre
  Store review-a, iako nije tehnički potreban za prvi build upload.
- [ ] Creator Homepage može posle Coming Soon stranice; nije release bloker.



## 3. Steam achievements

- [x] Definisano svih **27** API imena tačno po
  `[STEAM_ACHIEVEMENTS.md](STEAM_ACHIEVEMENTS.md)`.
- [x] Uploadovano 27 achieved + 27 locked ikonica.
- [x] Postaviti hidden flag za 6 endinga, `ACH_DEJA_VU` i
  `ACH_SPOT_PHANTOM`.
- [x] Publishovati Stats & Achievements promene.
- [ ] Testirati najmanje po jedan achievement iz svake grupe, zatim svih šest
  endinga i `ACH_SPOT_ALL` sa 9 anomaly tipova.



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

- [ ] Render env za parove modela: `AI_MODEL=gpt-5.6-terra` (tier `best`, otvara
  petlje 4+) i `AI_FALLBACK2_MODEL=gpt-5.6-luna` (tier `cheap`, otvara petlje 1–3),
  uz `AI_FALLBACK2_ENABLED=true`, URL i ključ. **Prazan `AI_FALLBACK2_API_KEY` tiho
  izbacuje ceo cheap tier** i sve petlje idu na primary, bez ijedne greške u logu.
- [ ] Potvrditi koji je `AI_MODEL` zaista aktivan na Renderu. Ako promenljiva tamo
  nije postavljena, važi commitovani `.env` default, pa se model menja samim
  deployom a ne svesnom odlukom.
- [ ] Ponoviti prompt QA za čist sprat i na kasnijoj petlji, ne samo na prvoj.
  Klijent za čist sprat šalje `anomaly_key="none"`, što je backend do 20.08.2026.
  čitao kao aktivnu anomaliju i forsirao osvetljeni lift tamo gde je mračni tačan.

- [x] `STEAM_APP_ID=4982260` i `STEAM_WEB_API_KEY` potvrđeni pravim auth zahtevom:
playtest build pokrenut iz Steam Library-ja dobio je ticket, sesiju i žive AI
odgovore (17.08.2026).

- [x] Nevalidan `AI_MODERATION_API_KEY` vraća HTTP 200, ali svaki odgovor postaje
  ista in-fiction fallback rečenica i chat provajder se nikad ne pozove. Kad
  „AI ne radi“ a igra ne prijavljuje grešku, prvo proveriti moderation ključ i
  `Content safety decision.` u logu.

- [x] Javni `https://loop9-backend.onrender.com/readyz` vraća
  `{"status":"ready"}` (provereno 07.08.2026.).
- [ ] Render Health Check Path postaviti/potvrditi kao `/readyz`.
- [ ] Pre javnog release-a ukloniti cold start: Render Starter ili ekvivalentan
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

- [~] Napraviti **Windows Shipping** build iz UE 5.8 posle content locka. Alfa
  Shipping build od 17.08.2026 je odigran preko Steama; finalni ide posle locka.
- [x] Proveriti da build ne sadrži:
  `steam_appid.txt`, pravi API ključ, game token, editor/debug sadržaj ili logove.
  `Builds/Alfa/Windows` (2.06 GB) nema `steam_appid.txt`, `*.pdb`, logove ni
  `Saved/`; jedini staged config je `Engine/Config/StagedBuild_Loop9.ini`.
- [ ] Pokrenuti Shipping EXE direktno na čistoj Windows mašini radi dependency
  provere.
- [x] Napraviti SteamPipe `app_build`/depot VDF i uploadovati Windows depot.
  Skripte su u `Tools/SteamPipe/`, depot `4982261`; sledeći upload je
  `Tools/SteamPipe/UploadPlaytest.bat`.
- [x] Postaviti build prvo na privatni `internal` ili `playtest` branch.
  BuildID `24782464` je live na passwordovanom `playtest`.
- [x] Instalirati build kroz Steam klijent, ne koristiti samo lokalni packaged
  folder. Shipping build pokrenut iz Explorera ne dobija Steam ticket, pa AI chat
  ne radi — QA se radi isključivo iz Library-ja.
- [ ] Posle QA postaviti odobreni build na default branch. Isti Alfa build je
  trenutno live i na `default`; bezopasno je dok igra nije released, ali finalni
  build tamo treba da ide svesnom odlukom (SteamCMD `setlive` ne može `default`).



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
- [ ] Steam Cloud: odigraj → izađi → druga mašina/obrisan lokalni save → progres se vrati.



### Platforma, UI i performanse

- [x] EN/SR/DE/FR/RU: meni, settings, chat, promptovi, ending i terminal.
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

- [ ] Publishovati sve Store Presence promene.
- [ ] Poslati Store Page na Valve review.
- [ ] Poslati release-candidate build na Valve review.
- [ ] Ispraviti eventualne review primedbe i ponovo poslati.
- [ ] Objaviti Coming Soon stranicu najmanje **14 dana** pre release-a.
- [ ] Steam Direct fee je plaćen 15.07.2026. Obavezni 30-dnevni Direct period
  ističe približno **14.08.2026**, ali to nije automatski release datum.
  Ako Coming Soon stranica nije bila javna do 07.08.2026, najraniji datum je
  najmanje **14 dana od njenog stvarnog objavljivanja** (najranije oko
  **21.08.2026.** ako se objavi 07.08.), uz završen Valve review i QA.



## 8. Release day

- [ ] Zamrznuti kod i sačuvati tačan commit/build ID koji ide live.
- [ ] Potvrditi `/readyz`, Redis, AI provajdere, quota alarm i Render kapacitet.
- [ ] Završiti Steam release proces i postaviti odobreni build live.
- [ ] Instalirati javni build sa drugog Steam naloga i uraditi 15-min smoke.
- [ ] Pratiti auth, chat, moderation, latency i 5xx logove tokom prvih sati.
- [ ] Imati prethodni stabilni depot/build spreman za rollback.



## 9. Post-release (nije launch bloker)

Radi se posle Coming Soon / live-a, samo ako ostane vreme. Ne blokira Valve review.

- [~] Ending session timeline: C++ kartice su gotove (`BuildRunEventCards()`,
  EN/SR/DE/FR/RU). Art je u `Content/MyStuff/UI/Timeline/`. Ostalo u editoru
  po MCP briefu: na `WBP_Ending_*` ostavi naslov + 2–3 rečenice, ispod spawnuj
  redove sa ikonicama. Bez grafa sirovih relationship brojeva.
- [~] Main-menu dosije / arhiva smena: C++ `ShiftArchiveWidget` čita `SeenEndings`.
  Ostalo u editoru: dugme tačno imena `Archive` na živom main-menu WBP-u;
  opciono `ui_archive_star_plate` kao pozadina + labele preko čvorova.
  Animaciju ne raditi. Brief:
  [`docs/HOME_EDITOR_TIMELINE_AND_ARCHIVE.md`](docs/HOME_EDITOR_TIMELINE_AND_ARCHIVE.md).
- [~] AI kontekst anomalija: C++ i backend su gotovi. Bez oznaka Dragojlo pošteno
  priznaje da ne zna gde je anomalija; sa oznakama ume da pošalje igrača u pravi
  deo sprata bez odavanja predmeta. Ostalo u editoru: popuniti `AnomalyZone` i
  `AnomalyObjectKind` na anomaly komponentama u `FullOfficeMap`
  (**Anomaly > AI Context**), na engleskom, orijentir + vrsta predmeta, nikad ime
  aktera; zonu ostaviti praznu za `PhantomMessage`. Može sprat po sprat.
  Pravila i primeri: [`docs/ANOMALIES.md`](docs/ANOMALIES.md#ai-context-tagging).

---



## Trenutni kritični put

1. Editor: elevator zvuci, ending mini-sequence polish, `AnomalyZone` oznake i
   brisanje `LS_Elevator_*` posle Reference Viewer-a.
2. Gameplay trailer — jedino što još realno blokira Store review.
3. Render: potvrditi aktivan `AI_MODEL`/`AI_FALLBACK2_*` par, health check `/readyz`
   i always-on plan; pa alarmi preko `/metrics`.
4. Ending balance QA (šest profila) + achievement test po grupi.
5. Novi Shipping build → `playtest` → QA iz Library-ja (Cloud, offline, gamepad).
6. Valve review → Coming Soon najmanje 14 dana → release.

