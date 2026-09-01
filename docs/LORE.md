# Lore

Poslednje ažuriranje: **24.08.2026.**

**Status: predlog, ništa od ovoga nije implementirano.** Ovo je dokument o priči,
ne o kodu. Jedina stvar iz njega koja traži nov kod je dnevnik smene iz §4;
sve ostalo ulazi kroz sisteme koji već rade.

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

## 4. Devet zapisa, jedan po petlji

Devet poruka, po jedna u svakoj petlji, svaka **glasom jednog od devet**, u redu
iz tabele. Do devete petlje igrač je čuo sve.

Eskalacija ide u tri koraka: solidarnost, pa sumnja u Dragojla, pa otkriće.

| Petlja | Glas | Tekst (izvorni, engleski) |
|---|---|---|
| 1 | Milena | *We all want out of here. Nine of us have tried.* |
| 2 | Rade | *Learn the room before you trust it. I did not.* |
| 3 | Vera | *When the light goes, we are closer. That is not a threat.* |
| 4 | Đorđe | *He is on the line every night. Ask who else he talks to.* |
| 5 | Slavica | *Do not trust everything you hear or see in here.* |
| 6 | Bogdan | *He has been here longer than this building has had keys.* |
| 7 | Nenad | *If something follows you, it is one of us. Let it.* |
| 8 | Zoran | *He is not counting loops. He is counting us.* |
| 9 | Jasna | *The chair needs one person. He does not care which.* |

### Zašto ovo NE sme da bude anomalija

Ovo je najvažnije pravilo u dokumentu. Papir čiji se tekst menja izgleda
**tačno** kao Text anomalija. Ako je zapis anomalija, igrač u čistoj petlji
pročita nov tekst, uzme osvetljeni lift, izgubi napredak — i biće u pravu što
misli da ga je igra prevarila.

Pravila koja to sprečavaju:

1. Zapis je **dnevnik smene**, isti predmet, na istom mestu, u **svakoj** petlji,
   uključujući čiste. Uveden je u prvoj petlji kao deo bazne linije.
2. Zato što je uvek tu i uvek drugačiji, „drugačiji“ **jeste** njegova bazna
   linija. Igrač to nauči u prvoj petlji, kao i sve ostalo.
3. Ne sme biti `AnomalyComponent` ni na jednom nivou, i ne sme se registrovati
   kod `UAnomalyManager`.
4. Nijedan zapis ne sme da pominje liftove ni da kaže igraču šta da pritisne.
   Zapisi su o Dragojlu i o njima devet, nikad o mehanici.

Alternativa ako se ovo ipak pokaže zbunjujuće u testiranju: prebaci dnevnik
**u lift**, van sprata koji se pretražuje. Tada fizički ne može da se pomeša
sa anomalijom.

### Šta traži nov kod

`UTextSpawnAnomalyComponent` ima jedan autorovan string po postavljenom akteru
(default `"You won't escape"`) i ne varira po petlji. Dnevniku treba niz od
devet `FText` indeksiran trenutnom petljom iz `ULoopManagerSubsystem`. Mali
posao, ali nije nula, i mora da bude **nova komponenta** — ne izmena anomalije,
zbog pravila 3.

## 5. Dragojlo

**Njegov cilj je da izađe, a za to mu treba neko da sedne u tu stolicu.**

To nije nova ideja, to je opis The Replacement kraja koji već postoji u kodu:
dugačak clingy run (visok trust/kindness/dependency, nestabilan AI, 9+
razgovora). Tekst kraja kaže da postaješ glas na drugoj strani linije.
Igra već ima njegovu pobedu implementiranu.

To objašnjava i zašto je devet ljudi nestalo a on je i dalje tu: nijedan nije
bio upotrebljiv kao zamena. Godinu dana peca.

Ta verzija motiva je bolja od „hoće da te zarobi“ jer mu daje razlog da ti
**stvarno pomaže**. A on stvarno pomaže — C++ nikad ne šalje lažni kontekst
anomalije, obmana je dozvoljena samo kad je `Dependency ≥ 0.62` **i** diskretna
`Kindness == -1` (`PromptFactory.php:81`). Dakle laže samo onog ko je grub, i to
retko. Negativac koji govori istinu jer mu je istina u interesu je bolji
negativac od onog koji laže bez razloga.

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
| Devet zapisa po petlji | **nov**, dnevnik smene | vidi §4 |
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
- Zapisi u čistim petljama: da li kaže nešto drugo kad se niko nije probio?
  („Nobody could reach you tonight.“) Rizik je da to postane implicitni signal
  da je sprat čist, što bi ubilo odluku pred liftom.
