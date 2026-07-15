# Editor TODO — šta ručno odraditi u Unreal editoru

Živi dokument: sve što je urađeno u C++/config-u, a traži ručni korak u
editoru (ili Steamworks-u) da bi proradilo. Kad nešto završiš, štrikliraj.
Novi zadaci se dodaju na dno odgovarajuće sekcije.

---

## 0. Prvo na kućnoj mašini (pre svega ostalog)

- [ ] `git checkout main && git pull` — sve od 15.07. je mergovano na main
  (lokalizacija, settings, telemetrija, 3 nove anomalije, marketing slike).
  Backend isto: `main` sadrži telemetry endpoint i nove promptove.
- [ ] Regeneriši project fajlove (desni klik na `Loop9.uproject` →
  *Generate Project Files*) — dodato je 10+ novih .cpp/.h fajlova
  (`Subsystems/Loop9GameSettingsSubsystem`, `Subsystems/Loop9TelemetrySubsystem`,
  `Anomaly/Scale|Clock|PhantomMessage...`, `Steam/Loop9SteamUtils`).
- [ ] Kompajliraj. Ako pukne na `Steamworks` dependency-ju u `Loop9.Build.cs`,
  javi — uslovno se linkuje samo na Win64/Linux/Mac.
- [ ] Redeploy backenda na Render (merge na main → auto deploy) da novi
  promptovi (10 anomalija) i `/api/telemetry/run` budu živi.

## 1. Settings widget (WBP_Settings ili kako se već zove tvoj widget)

C++ (`USettingsWidget`) sada ima opcione bindove — widgeti se automatski
povezuju **po imenu**, pa imena moraju biti tačna slovo-za-slovo:

- [ ] **`ComboBoxString_Language`** (ComboBoxString) — izbor jezika
  (English / Srpski). C++ ga sam popunjava i menja kulturu.
- [ ] **`Slider_Gamma`** (Slider, opseg 0–1) — C++ mapira 0–1 na gammu
  1.6–3.2. Stavi labelu "Brightness / Osvetljenje".
- [ ] **`Slider_MasterVolume`** (Slider, 0–1)
- [ ] **`Slider_MusicVolume`** (Slider, 0–1)
- [ ] **`Slider_SFXVolume`** (Slider, 0–1)
- [ ] **`Slider_MouseSensitivity`** (Slider, 0–1) — mapira se na 0.1–3.0.
- [ ] **`CheckBox_InvertY`** (CheckBox)

Ne treba nikakav Blueprint graf — binding (OnValueChanged) i učitavanje
sačuvanih vrednosti radi C++ (`BindValueWidgets` / `SyncWidgetsFromCurrentSettings`).
Vrednosti se pamte u `GameUserSettings.ini` i primenjuju pri sledećem pokretanju.

## 2. Audio: SoundClass asseti (za Music/SFX slajdere)

Master volume radi odmah. Za Music i SFX slajdere:

- [ ] Napravi `Content/Audio/SC_Music` i `Content/Audio/SC_SFX` (SoundClass).
- [ ] Svim muzičkim assetima dodeli Sound Class = `SC_Music`; svim
  zvukovima (telefon, koraci, ambijent, lift...) `SC_SFX`.
- [ ] U **`Config/DefaultGame.ini`** (tvoj lokalni, ne .example) dodaj:

```ini
[/Script/Loop9.Loop9GameSettingsSubsystem]
MusicSoundClassPath=/Game/Audio/SC_Music.SC_Music
SFXSoundClassPath=/Game/Audio/SC_SFX.SC_SFX
```

## 3. Lokalizacija (i18n)

Prevodi žive u repo-u kao PO fajlovi (kao na webu):
`Content/Localization/Game/sr/Game.po` — **srpski prevodi su već upisani**.
Pipeline config: `Config/Localization/Game.ini`.

- [ ] U svoj lokalni `Config/DefaultGame.ini` prekopiraj iz `.example`-a
  sekcije `[/Script/UnrealEd.ProjectPackagingSettings]` (CulturesToStage) i
  `[Internationalization]` (LocalizationPaths).
- [ ] Pokreni gather+compile (iz root-a projekta; prilagodi putanju enginea):

```bash
<UE_ROOT>/Engine/Binaries/Linux/UnrealEditor-Cmd \
  "$(pwd)/Loop9.uproject" -run=GatherText \
  -config="Config/Localization/Game.ini"
```

  Ovo generiše `Game.manifest`, `Game.archive`, kompajlira **`Game.locres`**
  (to igra čita u runtime-u) i osveži PO fajlove. Komituj sve što nastane u
  `Content/Localization/Game/`.
- [ ] Provera u editoru: Play → Settings → Language → Srpski. UI mora odmah
  da pređe na srpski (ako ne, locres nije kompajliran).
- [ ] Kad dodaš novi tekst u C++ (NSLOCTEXT/LOCTEXT): samo ponovo pokreni
  komandu, dopiši prevod u `sr/Game.po` (naći ćeš prazan `msgstr`), pa
  komandu još jednom.
- [ ] Blueprint/asset tekstovi (ako ih ima vidljivih igraču): isti pipeline
  ih hvata (`GatherTextFromAssets`), prevodi se dopisuju u isti PO.

## 4. Nove anomalije (dodavanje u nivo)

Tri nove komponente; sve rade kao postojeće (registruju se same u
AnomalyManager, imaju `AnomalyProbability`):

- [ ] **ScaleAnomalyComponent** — dodaj na 1–2 rekvizita (šolja, fascikla,
  stolica). Podesi `ScaleMultiplier` (1.25 = 25% veće; 0.8 = manje).
  Ništa drugo ne treba.
- [ ] **ClockAnomalyComponent** — treba ti zidni sat čije su kazaljke
  **odvojene komponente** na istom actoru. Nazovi komponente `MinuteHand` i
  `HourHand` (ili upiši svoja imena u properties komponente). Podesi
  `RotationAxis` prema tome kako je mesh kazaljke orijentisan (probaj
  Roll pa Pitch). Kazaljke idu unazad dok je anomalija aktivna.
- [ ] **PhantomMessageAnomalyComponent** — dodaj na AI_Friend (telefon)
  actor. Radi odmah: kad je aktivna, u chatu se pojavi poruka "Ti: ..." koju
  igrač nije poslao. Poruke možeš menjati u properties (localized).
- [ ] Test: konzolna komanda / debug način koji već koristiš za forsiranje
  anomalija (`ForceActivateAnyAnomaly` pokriva i nove tipove).

## 5. Steamworks (kad budeš u admin panelu)

- [ ] Dodaj 3 nova achievementa (tabela u `STEAM_ACHIEVEMENTS.md`, redovi
  25–27): `ACH_SPOT_SCALE` (Size Matters), `ACH_SPOT_CLOCK`
  (Counterclockwise), `ACH_SPOT_PHANTOM` (I Never Sent That — **hidden**).
- [ ] `ACH_SPOT_ALL` sada traži svih **10** tipova (kod je već ažuriran).
- [ ] Publish promene u Steamworks-u.

## 6. Backend (Render) — ništa u editoru, samo provera

- [ ] Backend promptovi sada opisuju 10 tipova anomalija — redeploy backenda
  (main → Render auto deploy).
- [ ] Posle prvog odigranog runa proveri u Render logs da stiže
  `Run telemetry.` zapis (ending, resets, aiMessages).

## 7. Steam store page — slike i screenshotovi

AI-generisani izvorni materijali su u **`Marketing/Steam/`** (commit u repo-u):
`main_capsule_source.png`, `library_capsule_source.png`, `hero_banner_source.png`,
`logo_source.png`. Treba ih iseći/skalirati na Steam dimenzije (GIMP/Photopea):

- [ ] **Header capsule 460×215** — crop iz `main_capsule_source.png`
- [ ] **Small capsule 231×87** — isti izvor, drži "LOOP 9" čitljivo i malo
- [ ] **Main capsule 616×353** — isti izvor
- [ ] **Vertical capsule 374×448** — crop iz `library_capsule_source.png`
- [ ] **Library capsule 600×900** — `library_capsule_source.png`
- [ ] **Library hero 3840×1240** — široki crop centra `hero_banner_source.png`
- [ ] **Library logo 1280×720 PNG sa providnošću** — iz `logo_source.png`
  izbaci crnu pozadinu (Select by Color → Delete)
- [ ] **Client icon 32×32 (.ico)** i **community icon 184×184** — samo "9"
  iz logoa

### Screenshotovi iz igre (min. 5, preporuka 8–10, 1920×1080, bez UI debug-a)

Predlog kadrova — redosled je i redosled utiska na stranici:

- [ ] **Hodnik sa dva lifta** — jedan osvetljen iznutra, drugi mračan; ovo je
  core mehanika i mora biti prvi screenshot.
- [ ] **Telefon/interkom sa otvorenim chatom** — vidljiva Dragojlova poruka
  (izaberi neku dramatičnu, npr. upozorenje na anomaliju). Ovo pokazuje
  AI featuru — glavni selling point.
- [ ] **Kancelarija u "čistom" stanju** — dnevna rutina, CRT monitori, papiri;
  baseline koji igrač uči.
- [ ] **Ista kancelarija sa vidljivom anomalijom** — npr. pomeren/uvećan
  objekat ili flicker svetla; idealno uporedivo sa prethodnim kadrom.
- [ ] **Pursuer momenat** — mračan hodnik sa siluetom u daljini (ne preblizu,
  ostavi misteriju).
- [ ] **Text anomalija** — natpis/tekst koji ne treba da postoji.
- [ ] **Chat sa phantom porukom** — "Ti:" poruka koju igrač nije poslao
  (dobar hook za opis ispod screenshota).
- [ ] **Loading ekran ili ending kartica** — ali NE spojlovati endinge;
  najbezbednije neka bude loading sa tipom.
- [ ] Snimaj u **shipping/high settings**, gamma na default (2.2), engleski
  UI (store je globalan), `t.MaxFPS 0`, sakrij FPS countere.
- [ ] Za trailer (kasnije): 30–60s screen-capture istih scena redom
  telefon → anomalija → lift odluka → rez na crno sa logom.

## 8. Smoke test posle svega

- [ ] Settings: gamma/volume/sensitivity/invert menjaju ponašanje i prežive
  restart igre.
- [ ] Jezik: Srpski prevodi vidljivi u interakcijama, chatu, endinzima,
  loading ekranu, settings opcijama.
- [ ] Nove anomalije se pojavljuju u rotaciji i tačna odluka ih "spotuje"
  (achievement toast).
- [ ] Phantom poruka: otvori chat dok je anomalija aktivna — poruka mora
  stajati u istoriji; pitaj Dragojla za nju — ne sme da te gaslight-uje.
