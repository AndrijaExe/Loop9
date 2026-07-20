# Loop 9 — elevator and ending sequence plan

Ovaj dokument deli posao između C++ orkestracije i ručnog Unreal Sequencer rada.
Cilj je da sekvence izgledaju filmski, ali da gameplay stanje nikada ne zavisi
isključivo od event tracka ili tačno pogođenog keyframea.

Status: C++ orkestracija je implementirana. Preostali Unreal MCP/editor koraci
su pretvoreni u copy/paste prompt:
[`UNREAL_MCP_SEQUENCE_HANDOFF.md`](UNREAL_MCP_SEQUENCE_HANDOFF.md).

## Šta već postoji

- `ALiftButton::Interact` odmah poziva
  `ULoopManagerSubsystem::OnElevatorButtonPressed`.
- `OnElevatorButtonPressed` odmah registruje odluku i poziva `AdvanceLoop` ili
  `ResetLoop`.
- `AdvanceLoop` / `ResetLoop` odmah teleportuju igrača na nasumični Exit point,
  generišu sledeću anomaliju i čiste chat.
- `ALiftDoorWing` već ume da otvara/zatvara vrata i tickuje samo dok se pomera.
- Ending trenutno zaključa input, uradi 2 s fade-to-black i prikaže odgovarajući
  ending widget; ne postoji world/Level Sequence kadar pre widgeta.

Zbog toga Sequencer ne treba samo nalepiti preko postojećeg toka: prvo se odluka
mora odvojiti od trenutka kada se menja loop i teleportuje igrač.

## Arhitektura

### C++ deo

Dodati world-scoped transition coordinator, preporučeno
`ULoopElevatorTransitionSubsystem` (`UWorldSubsystem`), sa stanjima:

1. `Idle`
2. `ClosingDoors`
3. `Travelling`
4. `Arriving`
5. `OpeningDoors`
6. `Completed`

Coordinator treba da:

- odbije drugi klik dok transition traje;
- sakrije interaction prompt i zaključa move/look/interact/pause;
- sačuva izabrani `EButtonType`, ali odluku registruje tačno jednom;
- zatvori vrata i/ili pokrene opcioni Level Sequence;
- tokom potpuno crnog/zaklonjenog kadra pozove commit loop odluke;
- teleportuje igrača na eksplicitni `LitElevatorArrival` marker, nikad na
  nasumičan Exit point za ovu tranziciju;
- otvori vrata osvetljenog lifta;
- vrati input i prompt čak i ako sequence asset nedostaje ili bude prekinut;
- očisti timer/delegate stanje na `Deinitialize`, map change i EndPlay;
- ima watchdog timeout da igrač ne može trajno ostati zaključan.

Za reprodukciju Level Sequence asseta modul treba da doda `LevelSequence` i
`MovieScene` dependencies u `Loop9.Build.cs` i da koristi
`ULevelSequencePlayer` sa `OnFinished`/`OnStop` cleanupom.

`ULoopManagerSubsystem` treba razdvojiti na:

- `ResolveElevatorDecision(ButtonType)` — odmah pravi immutable
  decision snapshot/token, ali još ne upisuje trajne statistike;
- `CommitElevatorDecision(Result)` — tokom blackouta tačno jednom beleži
  relationship/achievement odluku, menja loop broj, resetuje/generiše anomaly
  state i čisti chat;
- teleport izbaciti iz `AdvanceLoop` / `ResetLoop` za cinematic putanju ili ga
  kontrolisati eksplicitnim transition parametrom.

Ako sequence bude prekinut posle snimljene odluke, watchdog mora pozvati isti
world commit fallback; ne sme ostati zabeležena odluka bez završenog prelaza.

`ALiftButton::TryInteract_Implementation` treba da prosledi
`InteractingController` coordinatoru. Ako coordinator nije dostupan u test mapi,
može bezbedno koristiti postojeći instant fallback.

### Unreal Editor / Sequencer deo

Ti u editoru praviš:

- Level Sequence asset i Cine Camera kadar;
- camera transform/keyframeove;
- eventualni mali head turn ili camera shake;
- svetlo, zvuk motora, udar vrata i floor ding;
- precizan ritam otvaranja/zatvaranja;
- šest kratkih ending kadrova.

Gameplay odluka, save, achievement, teleport i input restore ostaju u C++.
Event Track može slati dekorativne cue događaje, ali ne sme biti jedini način da
se transition završi.

## Elevator transition — predloženi kadar

Ukupno: oko **4.5–5.5 sekundi**.

### 0.0–0.4 s — potvrda izbora

- Dugme se utisne i zasvetli.
- Input se zaključava.
- Kamera se za 0.25–0.4 s blago usmeri ka centru vrata izabranog lifta.
- Kratak električni klik; bez fadea.

### 0.4–1.8 s — zatvaranje vrata

- Vrata izabranog lifta se zatvaraju.
- Igrač i dalje vidi stvaran prostor; nema teleporta.
- Kamera ostaje first-person ili koristi vrlo blag Cine Camera blend.

### 1.8–3.2 s — putovanje

- Tek kada su vrata potpuno zatvorena: vrlo kratak fade ili potpuna zaklonjenost
  vratima.
- Arrival vrata se prvo drže zatvorena, da teleport nikada ne otkrije promenu
  lokacije.
- C++ commit-uje odluku i teleportuje igrača u **osvetljeni lift**, bez obzira
  koji lift je izabran.
- Reprodukuju se motor, metalno podrhtavanje i diskretan camera shake.
- Sledeći loop/anomalije moraju biti spremni pre otvaranja vrata.

### 3.2–5.0 s — dolazak

- Elevator ding.
- Fade se skida dok je igrač već u osvetljenom liftu.
- Osvetljena vrata se otvaraju.
- Input se vraća tek kada je prolaz bezbedno otvoren.

## Potrebni level elementi

- Jedan jasno imenovan `LitElevatorArrival` teleport marker sa rotacijom ka vratima.
- Reference na oba krila vrata izabranog lifta.
- Reference na oba krila vrata osvetljenog arrival lifta.
- Opcioni camera target unutar lifta.
- Opcioni elevator transition Level Sequence.
- Audio cuevi: button, close, travel loop/rumble, stop, ding, open.

U mapi postoji mnogo lift-door wing instanci, zato ih treba eksplicitno grupisati
po kabini (reference ili actor tag), a ne tražiti i zatvarati sve
`ALiftDoorWing` actore.

`ALiftDoorWing` treba da dobije movement-finished delegate. Coordinator čeka oba
krila, uz timer watchdog kao rezervu; ne treba pollingovati `ShouldMove` svakog
frejma.

Ne treba duplirati ceo sprat niti praviti posebnu mapu samo zbog 2–3 s putovanja.
Zatvorena vrata/fade su bezbedna granica za teleport u istoj mapi.

## Ending mini-sekvence

Current ending evaluator i widgeti ostaju izvor istine. Presenter treba da:

1. izračuna ending tačno jednom;
2. zaključa input;
3. potraži opcioni sequence za taj `ELoopEndingType`;
4. odigra sequence;
5. na `OnFinished` prikaže postojeći ending widget;
6. ako sequence ne postoji ili timeoutuje, koristi postojeći 2 s fade fallback.

Preporučeni asset mapping:

- `LS_Ending_EscapeTogether`
- `LS_Ending_ObedientFool`
- `LS_Ending_ColdBetrayal`
- `LS_Ending_ParanoidSurvivor`
- `LS_Ending_MergedMemory`
- `LS_Ending_TheReplacement`

Mapping može stajati u `ALoop9GameMode` kao
`TMap<ELoopEndingType, TSoftObjectPtr<ULevelSequence>>`, da asseti ne moraju svi
odmah biti hard-loadovani.

### Predlozi bez novih animiranih likova

#### Escape Together — 5–7 s

- Vrata lifta se otvore ka izlazu ili prejakom belom svetlu.
- Drugi set koraka prilazi van kadra.
- Dve senke prelaze preko zida; cut to ending card.

#### Obedient Fool — 4–6 s

- Kamera poslušno prilazi telefonu/terminalu.
- Svetla se gase redom iza igrača.
- Poslednji kadar je upaljena slušalica ili jedno dugme.

#### Cold Betrayal — 4–6 s

- Lift stane, ali se vrata ne otvore.
- Crveno/hladno svetlo i prekinuta telefonska linija.
- Nagli metalni udar iza kamere, zatim rez.

#### Paranoid Survivor — 3–5 s

- Igrač spušta/ostavlja slušalicu izvan kadra.
- Kamera se okrene od telefona ka jedinom izlazu.
- Telefon ponovo zazvoni tek u poslednjem crnom frejmu.

#### Merged Memory — 5–8 s

- Isti telefon/monitor se kratko pojavi u dve pozicije kroz light flicker.
- Tekst ili glasovna linija se preklopi bez prikazivanja novog karaktera.
- Lagani push-in i exposure bloom ka belom.

#### The Replacement — 6–8 s

- Kamera sedne/približi se Dragojlovom radnom mestu.
- Telefon zazvoni; prvi-person pogled pada na slušalicu.
- Posle podizanja ide postojeći Replacement terminal UI.

## Redosled izrade

1. C++ transition state machine i instant/failure fallback.
2. Jedan graybox elevator sequence bez finalnog zvuka.
3. Test: oba izbora, spam input, pause, map reload, ending na loopu 10.
4. Finalni elevator kadar, svetlo i zvuk.
5. C++ ending sequence mapping + fallback.
6. Prvo napraviti `TheReplacement` i `EscapeTogether` kao dva najrazličitija
   testa, zatim preostala četiri.
7. Finalni Shipping smoke svih šest endinga.

## Acceptance criteria

- Odluka, relationship brojači i achievementi menjaju se tačno jednom.
- Igrač se nikada ne teleportuje pred otvorenim vratima.
- Posle transitiona uvek završava u osvetljenom liftu okrenut ka izlazu.
- Dupli klik, pause i chat ne mogu pokvariti transition.
- Nedostajući Level Sequence koristi fallback i ne blokira run.
- Ending telemetry/achievement se šalju jednom, pre prikaza završnog UI-ja.
- Prekid sekvence ili map change ne ostavlja input zaključan.
- Sekvence rade na 30/60/120 FPS i ne zavise od broja Tick frejmova.
