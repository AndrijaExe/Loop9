# Lore

Poslednje ažuriranje: **01.09.2026.**

**Status: kanon iz §1–3 i Dragojlov cilj iz §5 ostaju radna osnova.**
Papiri iz §4 nisu napisani ni zaključani; Andrija ih osmišljava kasnije.
Prompt se za sada ne širi teom (devet duša, zamena, „ne veruj mu“) —
Dragojlo ostaje umoran kolega. AI modeli se ne menjaju: luna 1–3,
terra 4+.

Tekst koji ide u igru piše se **na engleskom**, jer je to izvorni jezik za
lokalizaciju. Ovaj dokument je na srpskom jer se o priči tako lakše priča.

---

## 1. Šta je već kanon i ne sme se protivrečiti

Ovo stoji u backend promptu, na store stranici i u trailer shotlisti. Nova
priča mora da se uklopi u ovo, a ne obrnuto.

| Činjenica | Gde je zapisano |
|---|---|
| Godina **2003.**, državna ustanova, kraj smene | `config/prompts/system_full.txt:1` |
| **Drugi sprat** se ponavlja, devet petlji | `STORE_PAGE.md`, `GAMEPLAY_SYSTEMS.md:5`, `TRAILER_SHOTLIST.md:265` |
| Linoleum, gomile papira, fluorescentna svetla | `system_full.txt:1` |
| Igrač je **noćni tehničar** | `HelpWidget.cpp` |
| Dragojlo, **oko 60**, drugo odeljenje, pred penzijom | `system_full.txt:1` |
| Nikad se ne pojavljuje lično, samo telefon / interfon | `system_full.txt:1` |
| **Milena** je razbila monitor pred kraj smene i ostavila ga tako | `system_full.txt:68` |
| Svetlo u stepeništu **trećeg sprata** ne radi nedeljama | `system_full.txt:68` |
| Radijatori rade i u avgustu | `system_full.txt:68` |
| Ime „Dragojlo“ se **ne prevodi** | `AI_ChatWidget.cpp:233` |

„9“ u naslovu je broj petlji, ne broj sprata. Sprat je drugi.

## 2. Premisa

Ustanova radi ono što svaka ustanova radi. Ljudi dolaze, ovaj sprat se prazni
u pola devet, i s vremena na vreme neko prestane da dolazi. Ne dramatično —
kao da je dao otkaz, odselio se i presekao svaki kontakt. Nema policije, nema
istrage, ostane samo prazan stol koji za dve nedelje neko drugi zauzme.

U prethodnih godinu dana to se dogodilo **devet puta**.

Njih devet nisu otišli. Zaglavili su u istoj petlji u kojoj je sada igrač, i
nijedan nije izašao. Ono što je od njih ostalo drži se sprata, i jedini način
na koji mogu da se pokažu je **kroz sopstveni najveći strah**. Zato anomalije
i postoje, i zato su tipizirane: svaki tip je jedan čovek.

Iz toga slede dve stvari koje priču drže zajedno:

**Anomalija je pomoć.** Igračev posao je da prepozna da sprat nije čist i uđe u
osvetljeni lift. Duša koja se manifestuje mu bukvalno govori tačan odgovor.
Ne rade to iz dobrote nego jer je to jedino što umeju — ali efekat je pomoć.

**Pomoć ima oblik terora.** Ne biraju kako izgledaju. Onaj koga je progonio
strah od jurnjave može da se pojavi samo kao Pursuer. Zato te „prijateljska“
anomalija juri kroz hodnik. To nije nedoslednost, to je poenta.

Čist sprat je petlja u kojoj nijedan nije uspeo da se probije. Dvadeset posto
petlji si sam.

## 3. Njih devet

Imena su srpska jer su Dragojlo i Milena već srpski i ustanova je državna, 2003.
Radno mesto je odabrano tako da **objašnjava strah**, a strah objašnjava tip
anomalije — bez toga mapiranje ispada nasumično.

| # | Ime | Radno mesto | Strah | Tip anomalije |
|---|---|---|---|---|
| 1 | **Milena Ristić** | referent | da ono što je pokvarila ne može da se vrati | Hide |
| 2 | **Radoslav „Rade“ Jovanović** | arhivar | zaveden papir koji se više nikad ne nađe | Move |
| 3 | **Vera Simić** | higijeničarka, noćna smena | hodnici kroz koje je sama prolazila | Light |
| 4 | **Đorđe Pantić** | telefonista na centrali | glasovi bez tela | Audio |
| 5 | **Slavica Kovač** | daktilografkinja | da joj se otkuca izmeni i pripiše | Text |
| 6 | **Bogdan Ilić** | portir, nosio je ključeve | da ga zatvore njegovim ključevima | DoorLock |
| 7 | **Nenad Vuković** | kurir | koraci za njim u stepeništu | Pursuer |
| 8 | **Zoran Babić** | pripravnik | da je ništa u mašini | Scale |
| 9 | **Jasna Petrović** | sekretarica | poruka poslata u njeno ime | PhantomMessage |

**Milena je namerno prva.** Ona je već u backend promptu, i Dragojlo o njoj
govori u prezentu — „razbila je monitor i ostavila ga tako“. Kad igrač shvati da
je Milena jedna od devet, ta rečenica se retroaktivno menja: on godinu dana
priča o njoj kao da je juče bila tu. Uz to, Replacement ending već prikazuje
monitor sa `NEW OPERATOR CONNECTED`. Razbijeni monitor i monitor koji prijavljuje
novog operatera su isti monitor.

## 4. Papiri na spratu (kako priča stiže do igrača)

Priča se ne spawnuje. **Nekoliko inspect papira već leži na spratu od
prve petlje**, na fiksnim stolovima / fascijama / pored telefona. Igrač ih
uzme kao svaki drugi predmet i pročita. Otkriće je što nije obišao taj sto,
ne što se papir pojavio.

How to Play to neće naučiti. Niko ga ne čita do kraja, i ne mora: Help već
kaže da prva petlja meri šta pripada, i da „ako je bilo na loop 1, pripada“.
Papiri moraju da budu na tom čistom spratu. Ko ih vidi tad, kasnije zna da
isti karbon nije anomalija. Ko ih ne vidi tad, vidi isti mesh na istom mestu
kao ostali kancelarijski nered.

### Radna ideja — nije zaključano

- Nekoliko listova, isti vizuelni jezik: žuti karbon / manila fasikla, 2003
  kancelarija. Ne lete, ne trepere, nemaju emissive.
- Inspect je **dokument**, ne crna soba sa rotacijom mesha. Levo slika
  papira, desno čitljiv transkript. Kao dokumenti u Rise of the Tomb
  Raider. Ostali predmeti (hefter, lampa) ostaju stari inspect.
- Tekst je `FText` (pet jezika). Eskalacija: solidarnost → sumnja →
  stolica. Prvi papir **nije** „Don't trust him“.
- Nijedan papir ne pominje liftove ni koji taster da se pritisne.
- **Nema `AnomalyComponent`.** Ne registruju se kod `UAnomalyManager`.
- **Ne spawnuju se** posle loop 1. Nov list koji nije bio na baznoj liniji
  **jeste** Text anomalija, i tada je osvetljeni lift tačan.

Text / Phantom anomalije i dalje smeju da vrište (*You won't escape*, glas
Jasne). To su anomalije. Ovi karbon listovi su nameštaj.

### Zašto spawn kvari odluku

Papir koji se „nekad pojavi“ izgleda tačno kao Text anomalija. Igrač na
čistom spratu pročita „Don't trust him“, uzme osvetljeni lift, resetuje se
— i u pravu je da ga je igra prevarila. Zato se listovi ne pojavljuju.
Stoje.

Ako u playtestu i dalje mešaju karbon sa Text anomalijom: isti listovi
u **kabini lifta**, van sprata koji se pretražuje. Ne How to Play.

### Vizuelni smer za inspect — za kasnije

Sadašnji inspect (`AInspectionStageActor`) vrti mesh u crnom. Za papire
to ne radi — 8pt na teksturi se ne čita, a `DisplayName` nema UI.

Mogući raspored, samo kad su `DocumentTitle` + `DocumentBody` popunjeni:

| Strana | Šta |
|---|---|
| Levo (~45 %) | Slika papira. Jedan deljeni karbon-okvir za prvi drop; kasnije može poseban sken (log centrale, inventar ključeva). Blagi nagib, bez puzzle rotacije. |
| Desno (~55 %) | Naslov, jedan red meta (ime / „2nd floor · carbon“), pa telo. Postojeći UI font, ne novi serif. |

Isti izlaz kao inspect (Esc / E / B). Igra pauzirana. Gamepad: desni
štap ne mora ništa. How to Play se ne širi. Ostali inspectables
diraju se.

### Tekstovi

Pune tekstove i konačan broj papira piše Andrija kad dođe do toga.
Ispod su samo teze za diskusiju, nisu kanon. Čist sprat ne menja tekst.

| # | Glas | Teza |
|---|---|---|
| 1 | Milena | Nine of us have tried. |
| 2 | Rade | Learn the room before you trust the labels. |
| 3 | Vera | When the light goes we are closer. That is not a threat. |
| 4 | Đorđe | He is on the line every night. Ask who else he talks to. |
| 5 | Slavica | Do not trust everything you hear or see in here. |
| 6 | Bogdan | He has been here longer than this building has had keys. |
| 7 | Nenad | If something follows you, it is one of us. Let it. |
| 8 | Zoran | He is not counting the nights. He is counting us. |
| 9 | Jasna | The chair needs one person. He does not care which. |

### Mogući tehnički obim — tek posle odluke

1. Na `UInspectableComponent` (ili tankoj document komponenti):
   `DocumentTitle`, `DocumentBody`, opciono `DocumentMeta` i
   `PaperImage`. Ako je body prazan, ostaje stari black-room inspect.
2. Novi WBP: levo papir, desno transkript. Pause, isti close. Font kao
   ostatak UI. GatherText, pet jezika.
3. Mesh + komponenta na izabranim mestima u `FullOfficeMap`. Bez
   anomaly komponente.
4. Ne dirati `UTextSpawnAnomalyComponent`. Anomalija ostaje anomalija.

Broj listova i da li je bolji jedan dnevnik ostaju otvoreni. Pravilo koje
ne sme da se prekrši: promena ili novi papir tokom petlje čita se kao
Text anomalija.

## 5. Dragojlo

Dragojlo ostaje umoran kolega na telefonu. Commitment sistem (pogrešna lokacija
jednom, pogrešan lift tek kasno i retko) živi samo u trenutnom runu i ne menja
kanon iz §1–3: on i dalje može da laže, ali ne sme da izmisli prostor koji mapa
nema, i loop 1 nikad ne laže.

**Njegov cilj je da izađe, a za to mu treba neko da sedne u tu stolicu.**

To nije nova ideja, to je opis The Replacement kraja koji već postoji u kodu:
dugačak clingy run (visok trust/kindness/dependency, nestabilan AI, 9+
razgovora). Tekst kraja kaže da postaješ glas na drugoj strani linije.
Igra već ima njegovu pobedu implementiranu.

To objašnjava i zašto je devet ljudi nestalo a on je i dalje tu: nijedan nije
bio upotrebljiv kao zamena. Godinu dana peca.

Ta verzija motiva je bolja od „hoće da te zarobi“ jer mu daje razlog da ti
**stvarno pomaže**. Većinu vremena kontekst anomalije je tačan. Kontrolisana
obmana (jedna pogrešna lokacija, najviše jedan pogrešan lift) ide iza
`AI_COMMITMENT_ENABLED` i samo duž zaključane zavisne putanje — ne sme da učini
Escape Together nedostižnim.

### Ne otkrivati ga prerano

Najozbiljniji rizik u celoj priči. Krajevi se biraju **isključivo** iz odnosa sa
njim, a `TotalAIInteractions < 3` odmah daje Paranoid Survivor. Ako igrač u
trećoj petlji shvati da je Dragojlo negativac i prestane da ga zove, **pet od
šest krajeva postaje nedostupno** i svi dobiju isti.

Zato: on ostaje uverljivo topao umoran čovek pred penzijom do kraja. Sumnja
stiže iz zapisa i iz anomalija, nikad iz njegovog tona. Tekst prompta se
**ne menja** u tom pogledu — samo se doda šta sme da pomene iz §3.

## 6. Kako se šest krajeva čita kroz ovu priču

Uslovi su iz `FLoopEndingEvaluator::Evaluate` (scoring od 01.09.2026.),
nisu izmišljeni. `< 3` razgovora i dalje tvrdo daje Paranoid Survivor;
inače se bira najbliži profil, ne prva AND kapija.

| Kraj | Mehanički uslov (skraćeno) | Šta znači u priči |
|---|---|---|
| **The Replacement** | 9+ razgovora, visok dep, topao odnos, nestabilan AI | Bio si mu topao *i* zavisan. Uspeo je. Ti si nova stolica, on izlazi. |
| **Escape Together** | 4–7 razgovora, pristojan, niska zavisnost, stabilan AI | Bio si dobar prema njemu ali ga nikad nisi trebao. Nema šta da iskoristi. Izlazite oba. |
| **Obedient Fool** | Često `DEPENDENCY=1`, 6+ razgovora, nije grub | Pustio si pogrešan glas da odluči umesto tebe. |
| **Cold Betrayal** | Prati ga, ali nizak kindness (~dve grube poruke) | Verovao si mu a bio grub. Slagao te je, i po pravilima je smeo. |
| **Merged Memory** | Ostao ljubazan, run je neredan (niska AI stability) | Postao si deseti. Ti i on ste se izmešali. |
| **Paranoid Survivor** | < 3 razgovora, ili visok suspicion / nizak trust | Nikad nisi podigao slušalicu. Nije imao gde da te uhvati. |

Teza igre ispada iz razlike između prva dva reda: **dobrota bez zavisnosti je
jedino što ga pobeđuje.** Isti topao odnos, jedina razlika je da li si ga
trebao — i to je razlika između njegovog izlaska i zajedničkog.

Napomena za balans: `TotalResets` (pogrešni liftovi) **ne ulazi** u izbor kraja.
Ako želiš da promašaji nešto znače za priču, to je nova veza koje sada nema.

## 7. Gde svaki tekst ulazi

Sve osim prvog reda već postoji i traži samo pisanje teksta.

| Sadržaj | Sistem | Fajl |
|---|---|---|
| Inspect papiri (karbon, od loop 1) | **nov**, document WBP (levo slika, desno tekst) | vidi §4 |
| Imena, precrtana imena, spisak zaposlenih | `UMaterialSwapAnomalyComponent` + `UInspectableComponent` | postavlja se u mapi |
| Poruke u telefonu „koje nisi poslao“ | `UPhantomMessageAnomalyComponent` | `PhantomMessageAnomalyComponent.cpp:12` — zameniti 4 defaulta glasom Jasne |
| Tekst koji se pojavi u prostoru | `UTextSpawnAnomalyComponent` | default `"You won't escape"` → glas njih devet |
| Dragojlove aluzije | backend prompt, `Optional office lore` bullet | `system_full.txt:68`, `system_compact.txt` |
| Zapisi o završenim smenama | Shift Archive | `ShiftArchiveWidget.cpp` |
| Kratke rečenice na učitavanju | `LoadingScreenWidget.cpp:11` | |
| Objašnjenje sveta igraču | Help ekran, `Loop9Help,*` | `HelpWidget.cpp` |

Sve što se doda mora kroz `LOCTEXT` / `NSLOCTEXT` i mora se prevesti na pet
jezika. Pravila: [LOCALIZATION.md](LOCALIZATION.md).

## 8. Otvorena pitanja

- Da li igrač ikada vidi ime **svog** prethodnika, ili je deseti bezimen?
- Da li se Milenin monitor može fizički naći na spratu, kao objekat koji igrač
  pregleda, pre nego što se pojavi u Replacement kraju?
- Da li `TotalResets` treba da uđe u izbor kraja, sad kad promašaj u priči znači
  da si nekog od njih devet propustio da vidiš?
- Zapisi u čistim petljama: **ne.** Isti tekst na istom listu. Poseban red
  za čist sprat bi rekao koji lift da uzmeš.
