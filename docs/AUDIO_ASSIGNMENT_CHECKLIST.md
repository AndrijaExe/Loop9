# Audio assignment checklist

Poslednje ažuriranje: **24.07.2026.**

Koristi ovo kao listu zvukova koje još treba da dodeliš u editoru
(Blueprint / map actor Details). C++ samo čita reference — bez asseta
slot ne svira.

Legenda: `[ ]` nije dodeljeno · `[~]` delimično / treba proveriti · `[x]` gotovo

---

## Elevator (`BP_LoopElevatorTransitionDirector` u `FullOfficeMap`)

- [ ] **Button Press Sound** — klik dugmeta na početku tranzicije
- [ ] **Travel Sound** — looping hum posle zatvaranja vrata (ne na ending-bound
  travelu koji je skraćen)
- [ ] Volume / fade: `Button Press Sound Volume`, `Travel Sound Volume`,
  `Travel Sound Fade Out Seconds`

## Ending mini-scene (`LoopEndingSceneDirector` u `FullOfficeMap`)

- [ ] **Footstep Sound** — Escape Together (dva koraka pred kraj)
- [ ] **Phone Ring Sound** — samo Obedient Fool (ambient ring tokom scene)
- [ ] **Line Cut Sound** — Cold Betrayal (prekid linije; ako prazno, nema fallback
  ring-a više)
- [ ] **Light Flicker Sound** — Merged Memory (dva kratka flicker pulse-a)

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

## Doors (`DoorInteractable` instance u mapi)

- [ ] **Locked Sound** — pokušaj zaključanih vrata
- [ ] **Open / Close Sound** — normalna vrata

## Pursuer anomaly (`PursuerAnomalyCharacter` / component)

- [ ] **Despawn Sound** (+ attenuation po želji)
- [ ] **Moving Murmur Loop Sound** (+ attenuation)
- [ ] **Active Anomaly Loop Sound** (na `PursuerAnomalyComponent`)

## Audio anomalies (svaki `AudioAnomalyComponent` u mapi)

- [ ] **Anomaly Sound** po instanci (šta se čuje kad je anomalija aktivna)
- [ ] Attenuation / volume / “play at location” po sceni

## Main menu (`BP_MainMenuGameMode` ili MainMenu mapa)

- [ ] **Main Menu Loop Sound** — looping ambient / muzika

## Settings / mix (nije asset slot, ali QA)

- [ ] Master / Music / SFX / Ambient slideri utiču na gornje kategorije
- [ ] Shipping build: nema tišine na critical pathu (lift, telefon, ending,
  pursuer, main menu)

---

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
