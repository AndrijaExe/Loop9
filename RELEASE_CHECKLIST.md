# Loop 9 — Steam Release Checklist

Praktičan spisak svega što mora da se odradi pre objave na Steamu, redosledom kojim ima smisla.
Povezani dokumenti: [STEAM_ACHIEVEMENTS.md](STEAM_ACHIEVEMENTS.md) (spisak i setup achievementa),
[ARCHITECTURE.md backend repo-a](../../Backend/Loop9_backend/ARCHITECTURE.md) (infrastruktura).

Legenda: `[ ]` nije urađeno · `[x]` urađeno · `[~]` delimično / u toku

---

## 1. Kod — blokeri

- [~] **E2E Steam auth test** — ticket → `/api/auth/steam` → session token → chat.
  U toku na kućnoj mašini (debug logovi + `.ToString()` ispravka u
  `Loop9AchievementsSubsystem.cpp` čekaju push). **Prvo pušovati kućne izmene,
  pa ovde `git pull`.**
- [x] **Kompajlirati igru** posle merge-a svih grana (lokalizacione izmene iz ove grane
  + kućne achievement izmene). (15.07. — Loop9Editor Development OK nakon
  `SettingsWidget` AddDynamic fix-a.)
- [ ] **Pravi App ID** — zameniti `SteamDevAppId=480` (Spacewar) pravim App ID-jem u
  `Config/DefaultEngine.ini` (lokalni fajl, nije u repou — vidi `DefaultEngine.ini.example`).
- [ ] Ako achievementi ne rade sa pravim App ID-jem a radili su na 480: proveriti da li
  `GetAuthTicketForWebApi` treba umesto `GetAuthSessionTicket` (napomena u STEAM_ACHIEVEMENTS.md).

## 2. Lokalizacija

Sav user-facing tekst u C++ je u `NSLOCTEXT`/`LOCTEXT`. Namespace-ovi:
`Loop9Endings`, `Loop9Terminal`, `Loop9Loading`, `Loop9Interaction`,
`Loop9Chat`, `Loop9Settings`, `Loop9Menu`. Nativni jezik: **engleski**.

Kulture u buildu: **en, sr, de, fr, ru** (`CulturesToStage` + `.locres`).
PO fajlovi: `Content/Localization/Game/<culture>/Game.po`.
Gather pipeline: `Config/Localization/Game.ini` (komanda u `EDITOR_TODO.md` §3).

In-game jezik: `SettingsWidget` — `ComboBoxString_Language` (C++ puni opcije),
`SetLanguage` / `GetCurrentCulture` (persist u `GameUserSettings.ini`).
Labeli settings/menija se grade iz C++ NSLOCTEXT (ne zavise od BP FText).

- [x] `AI_Friend` chat / ring / Answer / low-signal — lokalizovano
- [x] AI jezik prati UI kulturu (`PreferredLanguage` opcioni override)
- [x] Settings + main/pause dugmad lokalizovani iz C++ (`Loop9Settings` / `Loop9Menu`)
- [x] GatherText + `.locres` za en/sr/de/fr/ru
- [x] Smoke: promena jezika odmah + posle restarta (settings, chat, endingi, loading)
- [ ] Opciono: dopuniti prazne asset `msgstr` u PO (ako neki BP tekst još curi)
- [ ] `LoopNumberSign` ("LOOP 9") — namerno diegetski engleski

## 3. Steamworks backend (partner.steamgames.com)


- [~] **Steam Direct fee plaćen 15.07.2026.** Banka (NLB Komercijalna) i W-8BEN
  uneti; **identity verification u toku (2–7 radnih dana)**. Tek posle toga
  stiže App ID i pristup store page alatima.
  - 30-dnevni tajmer teče od 15.07. → najraniji mogući release **~14.08.2026.**
  - Coming Soon stranica mora biti javna **min. 2 nedelje** pre release-a.
- [ ] Kad stigne App ID: upisati ga u lokalni `Config/DefaultEngine.ini`
  (umesto 480) i uzeti **Web API Key** → Render env (`STEAM_APP_ID`,
  `STEAM_WEB_API_KEY`).
- [ ] **Achievements**: definisati svih **27** po tabeli iz STEAM_ACHIEVEMENTS.md
  (API imena moraju biti identična), upload ikonica (27 × otključana + zaključana).
  Nova 2 (15.07.): `ACH_SPOT_SCALE`, `ACH_SPOT_PHANTOM` (hidden);
  `ACH_SPOT_ALL` sada traži 9 tipova anomalija.
- [ ] **Store page**: opis (EN + SR), screenshotovi, trailer, capsule slike, tagovi
  (Horror, Psychological, Time Loop, AI), obavezno **AI disclosure** polje — igra koristi
  generativni AI u gameplay-u (Valve to zahteva od 2024).
- [ ] **Depots & Builds**: napraviti depot za Windows build, upload preko `steamcmd`
  (`app_build` skripta) ili SteamPipe GUI; postaviti default branch.
- [ ] **Launch options**: putanja do exe-a.
- [ ] Iz shipping builda **ne pakovati** `steam_appid.txt` (samo za lokalni development).
- [ ] **Steam Cloud (Auto-Cloud)** — čuvanje progresa bez koda:
  - Root: `WinAppDataLocal`, putanja: `Loop9/Saved/Config/Windows/`,
    pattern: `Game.ini` (viđeni endinzi, spotted anomalije, player GUID)
    i po želji `GameUserSettings.ini` (grafika/jezik).
  - Testirati: odigraj → izađi → obriši lokalni fajl → pokreni → progres se vratio.
- [ ] **Cena** — po analizi iz pricing canvas-a; postaviti regionalne cene (Valve matrix).

## 4. Backend / Render (potvrđeno 15.07.)

- [x] `/healthz` 200, auth odbija loše tokene, Redis radi, Steam kredencijali podešeni.
- [x] **`/readyz`** — dependency-aware readiness (config + Redis rate-limiter storage);
  `/healthz` ostaje lagani liveness probe.
- [ ] Render **Health Check Path** potvrditi kao `/readyz` (ne `/healthz`) pre deploy-a.
- [x] Production fail-closed defaults: Steam-only (`AUTH_ALLOW_GAME_TOKEN=false`),
  body cap 64 KiB, AI total deadline 45s, redacted provider logs, PHPUnit pre Render deploy.
- [ ] **Free tier cold start (~15s)**: pre release-a preći na plaćeni Render plan
  (Starter) ili dodati keep-alive ping — prvi API poziv novog igrača ne sme da visi 15s.
- [ ] Podesiti alarm/notifikaciju za `GAME_GLOBAL_DAILY_QUOTA` (kill-switch na 5000 msg/dan).
- [x] `AUTH_ALLOW_GAME_TOKEN` isključen u prod defaults (legacy token samo preko
  eksplicitnog non-prod config-a / `.env.test`).

## 5. QA pre uploada builda

- [ ] Shipping / packaged **Steam** build na **čistoj mašini** (bez UE, bez dev fajlova).
- [ ] **Cold launch → first message auth**: Standalone (ne PIE) sa Steam klijentom —
  prva poruka čeka session (authorize-then-dispatch), ne šalje unauthenticated request;
  auth failure / offline refunduje pokušaj + in-fiction poruka.
- [ ] Steam auth E2E na pravom App ID-ju (novi Steam nalog koji poseduje igru).
- [ ] **Loop 9 boundary**: context `loop_index` clamp 1–9 na clientu i backendu.
- [ ] **Telemetry / relationship counts**: `RegisterPlayerMessage` na submit;
  `RegisterAIInteraction` / achievement / `ai_messages` telemetrija samo posle
  uspešnog validiranog AI odgovora (ne na failed/timeout).
- [ ] Svih 6 endinga dostižno; achievementi se otključavaju (proveriti u Steam profilu).
- [ ] **Offline test**: pokreni igru bez interneta — igra ne sme da pukne; chat prikazuje
  in-fiction poruku ("...the line crackles and goes dead...") i vraća potrošeni pokušaj
  za taj loop, pa igrač može odmah da proba ponovo.
- [ ] **Cold start / timeout test**: client chat timeout **65s** (backend AI deadline 45s
  + cold-start headroom); poruka mora stići ili failati sa lokalizovanom greškom, ne tišina.
- [ ] **Perf smoke (home machine)**: 10-min Unreal Insights capture + `stat unit` /
  `stat game` / `stat gpu` na desktopu i Steam Deck targetu. Renderer (Lumen/RT/VSM)
  ne dirati dok Insights ne dokaže bottleneck.
- [ ] Alt-Tab / Steam Overlay (Shift+Tab) ne ruši igru.
- [x] Promena jezika u settings-u menja UI odmah i posle restarta;
  Back/Resume ne ostavljaju settings overlay u pozadini.
- [x] Audio/gamma/sensitivity podešavanja rade i pamte se
  (`Loop9GameSettingsSubsystem`: Master + Ambient preko sound-mix override, bez
  direktne mutacije `USoundClass` asseta).
- [x] Telemetrija: run ping stiže (`Telemetry POST` + HTTP 204 u client logu).
- [ ] Verifikovati da build ne sadrži `DefaultGame.ini` sa pravim tokenima u repou
  (gitignore već pokriva, ali proveriti pakovani build).
- [x] **Scale + Phantom** u nivou (smoke OK); Clock uklonjen iz scope-a.

## 6. Steam Deck

- [x] **On-screen tastatura**: kad chat input dobije fokus na Deck-u, igra poziva
  `ShowFloatingGamepadTextInput` preko `FLoop9SteamUtils` (no-op van Steama).
  Modul sada linkuje Steamworks SDK direktno (`Loop9.Build.cs`, `LOOP9_WITH_STEAM`).
- [x] **Gamepad bindinzi**: `IMC_Default` iz šablona već pokriva move/look/jump
  (Gamepad_Left2D / Right2D / FaceButton_Bottom). Za keyboard-only akcije igra u
  runtime-u dodaje `IMC_GamepadFallback` kontekst: Interact → **X/Square**,
  Pause → **Start/Menu**, Sprint → **klik levog stika**. Bez izmena asseta.
- [ ] QA na pravom Deck-u (ili preko Steam Input simulacije): kretanje, interakcija,
  pauza, kucanje u chatu preko floating tastature; `stat unit` / `stat gpu`.
- [ ] Ako želiš drugačiji raspored dugmadi, izmene su u
  `Loop9Character::AddGamepadFallbackMappings` / `HorrorCharacter` override-u.

## 7. Poznate rupe posle launcha (backlog)

- [x] Offline/error UX za chat — na grešku se prikazuje in-fiction poruka (offline /
  zauzeto / greška servera), potrošeni pokušaj se refundira, HTTP timeout 65s;
  Steam session gate pre chat dispatch.
- [x] Achievement progres — Steam "x/y" progress toast preko
  `IndicateAchievementProgress` za ACH_STREAK_7, ACH_GROUNDHOG, ACH_HOTLINE,
  ACH_ALL_ENDINGS i ACH_SPOT_ALL. Čisto vizuelno; izvor istine ostaje `Game.ini`.
  (Puni Steam stats sa server-side čuvanjem i dalje backlog — zahteva definisanje
  statova u Steamworksu i migraciju.)
- [x] Lokalizacija — C++ + settings/meni + smoke OK; opciono asset PO stringovi.
