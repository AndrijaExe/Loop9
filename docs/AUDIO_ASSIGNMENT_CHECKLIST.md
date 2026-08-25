# Audio assignment checklist

Poslednje ažuriranje: **24.08.2026.**

Koristi ovo kao listu zvukova koje još treba da dodeliš u editoru
(Blueprint / map actor Details). C++ samo čita reference — bez asseta
slot ne svira.

Legenda: `[ ]` nije dodeljeno · `[~]` delimično / treba proveriti · `[x]` gotovo

---

## Elevator (`BP_LoopElevatorTransitionDirector` u `FullOfficeMap`)

- [x] **Button Press Sound** — klik dugmeta na početku tranzicije:
  `Sound/Elevator/ElevatorButtonPress` (mono, 44.1 kHz, 0.194 s, peak −3 dBFS).
  Dodeljen i na class defaults BP-a i na instancu u `FullOfficeMap`.
- [x] **Travel Sound** — looping hum posle zatvaranja vrata (ne na ending-bound
  travelu koji je skraćen): `Sound/Elevator/ElevatorTravelLoop`
  (stereo, 44.1 kHz, 6.000 s, `Looping` = **true** na SoundWave-u,
  peak −6 dBFS / RMS ≈ −19.7 dBFS). Bez fade-a u fajlu — gasi ga
  `Travel Sound Fade Out Seconds`.
- [~] Volume / fade: `Button Press Sound Volume` = 1.0,
  `Travel Sound Volume` = 1.0, `Travel Sound Fade Out Seconds` = 0.15.
  Slotovi su sada puni, pa se prvi put mogu čuti u igri. Predlog za prvo
  slušanje: hum je namerno prisutan bed, ako je pretežak spusti
  `Travel Sound Volume` na ~0.6–0.8; klik je normalizovan na −3 dBFS pa
  ~0.7 ako zvuči prejako u kabini.
- [ ] Travel loop prestaje na normalnom dolasku, timeoutu, abortu, ending
  handoffu i izlasku iz mape; ponovljene vožnje ne slažu više loopova.
- [ ] Klik dugmeta se čuje samo kada je tranzicija prihvaćena, ne kada igrač
  pritisne dugme izvan kabine ili tokom već aktivne tranzicije.

## Ending mini-scene (`LoopEndingSceneDirector` u `FullOfficeMap`)

- [x] **Footstep Sound** — Escape Together (dva koraka pred kraj):
  `Sound/Footsteps/Footstep`
- [x] **Phone Ring Sound** — samo Obedient Fool (ambient ring tokom scene):
  `Sound/Phone/PhoneRingingSound`
- [x] **Line Cut Sound** — Cold Betrayal (prekid linije; ako prazno, nema fallback
  ring-a više): `Sound/Phone/PhoneLineCut` (mono, 44.1 kHz, 0.624 s).
  Struktura: ~0.26 s tihog šuma otvorene linije (telefonski pojas 300–3200 Hz),
  pa tvrd mehanički klak, pa linija prestaje bez fade-a → mrtva tišina.
- [x] **Light Flicker Sound** — Merged Memory (dva kratka flicker pulse-a):
  `Sound/MainMenu/light-flicker`

## Replacement terminal (WBP / `ReplacementTerminalWidget` class defaults)

- [ ] **Typing Sound** — typewriter klikovi tokom zelenog terminala
- [ ] **Prompt Complete Sound** — posle hold-a na `You:_`, pre ending kartice

## Phone / Dragojlo (`BP_AI_Friend`)

- [ ] **Initial Ring Sound** — auto-ring na BeginPlay (ako je uključen)
- [ ] **Phone Interact Sound** — interakcija / pickup fallback
- [ ] **Phone Answer Sound** — kad igrač “javi” telefon

## AI chat UI (`AI_ChatWidget` / WBP chat)

- [ ] **Typing Sound** — kucanje igrača
- [ ] **AI Mumble Sound** — normalan odgovor
- [ ] **AI Mumble Anomaly Sound** — nestabilan / anomaly odgovor

## Player (`BP_HorrorCharacter` / Loop9 character)

- [ ] **Footstep Sounds** (niz) — hodanje po kancelariji
- [ ] **Footstep Volume** — provera u igri
- [x] **Flashlight Toggle Sound** (`Audio|Flashlight`) — klik lampe na `F`.
  Generisan `Sound/Flashlight/FlashlightToggle` (mono, 44.1 kHz, 0.11 s,
  peak −8 dBFS, dva latch transijenta). C++ default na `Loop9Character`;
  ako asset još nije uvezen, fallback je `Sound/Elevator/ElevatorButtonPress`.

## Light flicker anomalije (`LightFlickerAnomalyComponent`)

- [x] **Flicker Sound** — default u C++ konstruktoru je
  `Sound/MainMenu/light-flicker`, isti asset koji koristi Merged Memory ending.
  Svira se na poziciji **light komponente**, ne actor pivota, da bi igrača vodio
  ka pravom mestu. Ne mora ništa da se dodeljuje po instanci; prazan slot znači
  nemo treperenje.
- [x] **Flicker Sound Attenuation** — `Sound/MainMenu/ATT_FlickerHallway`
  (inner 300 cm, falloff 1500 cm, silent at 18 m, Linear, LPF, bez occlusion da
  plafon ne uguši klik). Dodeljen na `BP_Light_17` / `_35` / `_37` i kao C++
  default na `LightFlickerAnomalyComponent`. Hodničko svetlo pored lifta i dalje
  sme da se čuje iz kabine; svetla u back room / tool shelf ne.
- [ ] Volume / tajming po instanci — C++ defaulti su `Flicker Sound Volume` = 0.7,
  `Flicker Sound Trigger Level` = 0.3 (dip ispod kog se pali),
  `Min Seconds Between Flicker Sounds` = 0.5 + jitter do 0.9 s.
  Ako zvuči kao mitraljez, digni min pauzu; ako se ne poklapa sa slikom, digni
  trigger level.

## Doors (`DoorInteractable` instance u mapi)

C++ defaulti učitavaju `Sound/Doors/DoorOpen|Close|Locked|Blocked` (generisani
WAV u `Tools/make_door_and_pursuer_sfx.py`). Posle importa u editoru označi
**Looping = false** na door clipovima.

Odskrinuta vrata: na instanci (`BP_Door` / `BP_Door2`) uključi **Blocked From
Behind** (`bIsBlocked`) i isključi **Locked**. Prompt je
`Blocked by something behind` / sr `Nešto blokira s druge strane`. DoorLock
anomalija preskače blocked vrata.

Uz zvuk, vrata se i **zatresu** kad ne mogu da se otvore: kratak šut oko rest
rotacije, amplituda opada da se ne zaustavi u pola trzaja. Po instanci u
kategoriji `Door|Rattle`.

- [x] **Locked Sound** — `Sound/Doors/DoorLocked` (C++ default)
- [x] **Blocked Sound** — `Sound/Doors/DoorBlocked` (C++ default)
- [x] **Open / Close Sound** — `Sound/Doors/DoorOpen` i `DoorClose` (C++ default)
- [x] **Rattle** — C++ defaulti su `Rattle Angle Deg` = 2.0,
  `Rattle Duration Seconds` = 0.35, `Rattle Shakes Per Second` = 13.
  Isključi preko `bRattleWhenDenied` ako neka vrata ne smeju da se pomeraju.
- [ ] Import WAV → uasset u editoru pre sledećeg Shipping cooka
- [ ] Na odskrinutim vratima: `bIsBlocked = true`, `bIsLocked = false`

## Pursuer anomaly (`PursuerAnomalyCharacter` / component)

Dok je figura živa, level `AAmbientSound` se gasi i svira 2D tension bed.
Čim pursuer despawnuje, vraća se obična ambient muzika. Murmur je isti clip
kao Dragojlo na telefonu (`MumblingCrazy`), spatial, samo dok se kreće i dok
igrač ne gleda u njega.

- [x] **Despawn Sound** — `Sound/Pursuer/PursuerDespawn` (C++ default)
- [x] **Moving Murmur Loop Sound** — `Sound/Phone/MumblingCrazy` +
  `ATT_PursuerFootstep` (C++ default; clip se ponavlja dok hoda)
- [x] **Active Anomaly Loop Sound** — `Sound/Pursuer/PursuerTensionLoop` ako je
  uvezen, inače fallback `Sound/MainMenu/HorrorAmbientSound`. Posle importa
  tension WAV-a stavi **Looping = true**.

## Audio anomalies (svaki `AudioAnomalyComponent` u mapi)

- [x] **Anomaly Sound** po instanci — `telephone` i `telephone2` sviraju
  `Sound/Phone/PhoneRingingSound`.
- [x] Attenuation / volume / “play at location” — oba telefona su spatial, volume
  0.2, `ATT_PhoneRinging` inner 180 cm / falloff 820 cm (**silent at 10 m**).
  Lift je ~13.5–17 m od oba aparata, pa ring više nije čujan iz kabine; moraš
  ući u kancelariju. Occlusion + LPF su uključeni da zidovi dodatno uguše.
  Isti asset je C++ default na `AudioAnomalyComponent`.

## Ending typing (`EndingWidget` class defaults, `Ending|Typing`)

Zadnji red na ending ekranu se iskucava kao da ga Dragojlo piše, kroz isti
`FTypewriterHelper` koji koristi replacement terminal.

- [x] **Typing Sound** — `Sound/UI/TypewriterKey` (C++ default, generisan u
  `Tools/make_typewriter_key.py`). Pitch se randomizuje 0.92–1.08 po znaku, pa
  jedan clip ne zvuči kao mitraljez. Posle importa: **Looping = false**,
  **Sound Class = `SC_SFX`** (ne `SC_Music`, da ne ide kroz ambient slider).
- [x] **Typing Characters Per Second** = 20, **Typing Typo Probability** = 0.02
  (povremeno omaši slovo pa ga ispravi), **Typing Sound Volume** = 0.55.
- [ ] Ako `TypewriterKey` nije uvezen: tekst se i dalje iskucava, samo bez zvuka.

## Level music (`BP_Loop9GameMode > Audio`)

Muziku na spratu pušta game mode kao 2D zvuk, a ne `AAmbientSound` u mapi.
Postavljeni `AmbientSound_0` na `SC_Music` se pri startu utiša da se track ne
duplira; ostali ambient akteri se ne diraju.

- [x] **Level Music Sound** — `Sound/Ambient/HorrorAmbience1` (C++ default)
- [x] **Level Music Volume** = 0.35, množi se ambient sliderom
- [ ] QA: muzika radi od ulaska u nivo, bez otvaranja settings menija. Ako ne
  radi, `AudioStatus` u konzoli ispisuje device / jačine / bed / ambient aktere.

## Main menu (`BP_MainMenuGameMode` ili MainMenu mapa)

- [ ] **Main Menu Loop Sound** — looping ambient / muzika

## Settings / mix (nije asset slot, ali QA)

- [ ] Master / Music / SFX / Ambient slideri utiču na gornje kategorije
- [ ] Shipping build: nema tišine na critical pathu (lift, telefon, ending,
  pursuer, main menu)
- [ ] Pause/resume, Alt-Tab i promena output uređaja ne ostavljaju zaglavljen
  loop niti dupliraju muziku/ambient.

---

## Licence uvezenih zvukova (obavezno pre release-a)

Sva tri zvuka uvezena 21.08.2026. su izvedena isključivo iz **CC0 / public
domain** izvora. **Nijedan ne zahteva atribuciju** — ne mora ništa da ide u
credits. Ako se ipak želi „thanks to“ sekcija, imena autora su niže.
Nijedan izvor nije NonCommercial ni ShareAlike.

| Asset | Izvor (URL) | Autor | Licenca | Šta je urađeno |
|---|---|---|---|---|
| `Sound/Elevator/ElevatorButtonPress` | https://opengameart.org/content/87-clickety-clips (`Click_Clips.zip` → `click70.wav`) | OwlishMedia | CC0 1.0 | trim na transijent, stereo→mono, peak −3 dBFS |
| `Sound/Elevator/ElevatorTravelLoop` | https://opengameart.org/content/the-shop (`legit_audio_-_the_shop_free_sfx_wav.zip` → `TheShopCollection_convenience_store_drinks_fridge_drone.wav`) | LEGIT Audio | CC0 1.0 | najstabilnijih 6.9 s, 96→44.1 kHz, equal-power crossfade preko šava, peak −6 dBFS |
| `Sound/Phone/PhoneLineCut` (klak) | https://opengameart.org/content/100-cc0-sfx (`100-CC0-SFX.zip` → `switch_01.ogg`) | rubberduck | CC0 1.0 | trim, mono, 44.1 kHz |
| `Sound/Phone/PhoneLineCut` (šum linije) | https://opengameart.org/content/frequency-static-sound-effects (`static4.wav`) | bretbernhoft | CC0 1.0 / PD | isečak, band-pass 300–3200 Hz, −20 dBFS, tvrd rez bez fade-a |

Napomene:

- „The Shop“ je CC0 **samo** za besplatne semplove objavljene na OpenGameArt-u;
  plaćena LEGIT Audio biblioteka ima drugu licencu. Koristi se OGA verzija.
- Sirovi `.wav` fajlovi žive **u** `Content/MyStuff/Sound/...` pored `.uasset`-a
  jer su uvezeni preko editorskog *Monitor Content Directories* mehanizma.
  **Ne brisati ih** dok je auto-import uključen: `bAutoDeleteAssets=True` bi
  obrisao i SoundWave. Ako se sirovi fajlovi izbacuju iz repoa, prvo isključi
  auto-reimport (Editor Preferences → Loading & Saving → Auto Reimport).
- Build skripta koja je napravila fajlove iz izvora nije u repou (radila je u
  `Saved/`); tabela iznad je dovoljna da se rezultat ponovi.

### Kako je potvrđeno da `ElevatorTravelLoop` loop-uje bez šava

Merenja na PCM sadržaju finalnog fajla (bez slušanja):

- Skok između poslednjeg i prvog sempla = 0.0048, dok je prosečan skok
  između susednih sempala *unutar* fajla 0.0032 a 99.9. percentil 0.0203 —
  šav je unutar normalnog kretanja signala, daleko od klika.
- Skok envelope-a (30 ms) na mestu šava je **manji** od 53 % svih takvih
  skokova unutar fajla.
- Spektralna razlika levo/desno od šava: 6.54 dB prosečno; ista mera između
  dva susedna *unutrašnja* prozora istog materijala: 6.49 dB (5.93–7.95).
  Šav se dakle statistički ne razlikuje od bilo koje druge točke u fajlu.
- Blok-RMS (100 ms): sd 0.93 dB, prve i poslednje 300 ms su +0.16 dB /
  −0.64 dB u odnosu na srednju vrednost → **nema fade-a zapečenog u fajl**.

## Ending-specific quick map

| Ending | Očekivani zvukovi |
|---|---|
| Escape Together | Footsteps (×2) |
| Obedient Fool | Phone ring (ambient) |
| Cold Betrayal | Line cut |
| Paranoid Survivor | (nema ending-director audio; samo lift + vizuelni glimpse) |
| Merged Memory | Light flicker (×2) |
| The Replacement | Terminal typing + Prompt Complete Sound |

## Paranoid glimpse (nije audio, ali često zaboravljeno)

- [ ] Na elevator directoru: **Paranoid Glimpse Actor** ili **Paranoid Walker Class**
  (radi i za svetli i za tamni lift — koristi vrata dugmeta koje je pritisnuto)

## Escape Together companion

- [ ] Na ending directoru: **Escape Together Companion Actor** (sakriveni Dragojlo mesh)
  ili **Escape Together Companion Class** — stoji pored igrača u liftu
