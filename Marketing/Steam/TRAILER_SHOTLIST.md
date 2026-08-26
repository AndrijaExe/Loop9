# Loop 9 — trailer shot-list (snimanje + montaža)

Steam App ID: **4982260**
Cilj: gotov trailer za Store review. **Ispunjeno 22.08.2026.** — trailer je
uploadovan i Valve review je u toku. Vidi `RELEASE_CHECKLIST.md` §2 / §7.

Ukupno trajanje: **68 sekundi**.
Broj klipova koje snimaš: **12** (svaki snimi 10–20 s, u montaži koristiš samo
najbolje 3–7 s).
Alat za montažu: **CapCut** — samo rezovi, fade-ovi i tekstualne kartice.
Bez motion graphics-a, bez kompozitinga, bez keyframe-ovanih efekata.

> Sav tekst koji ide **na ekran** je na engleskom, jer je store globalan.
> Ovaj dokument je uputstvo za tebe i ostaje na srpskom.

---

## 0. Tri pravila koja se ne pregovaraju

### 0.1 Samo pravi gameplay

U traileru sme da bude **isključivo snimak stvarne igre iz stvarnog builda**.
Nema generativnog AI videa, nema izmišljenih „cinematic" kadrova, nema
montiranih scena kojih nema u igri, nema render-a iz editora koji ne
predstavlja ono što igrač zaista vidi.

Razlog nije estetski. Valve traži da trailer predstavlja stvarnu igru, a Loop 9
**već nosi AI-content disclosure na store stranici** (runtime AI dijalog +
AI-assisted marketing art, `STORE_PAGE.md` → „AI disclosure"). Aplikacija sa
već prijavljenim AI sadržajem koja pošalje i fabrikovan video materijal je
realan rizik za odbijanje na review-u, i taj rizik nije vredan nijednog
lepšeg kadra. Jedina stvar u traileru koja nije gameplay je poslednji frame
sa logom, i to je postojeći asset iz `StoreUpload/`.

### 0.2 Prve 3 sekunde moraju da rade bez zvuka

Steam pušta trailere na store stranici **automatski i mutirano**. Igrač koji
skroluje vidi prve 2–3 sekunde bez ijednog tona. Zato Klip 1 nije ambijent i
nije logo — to je jedini kadar u igri koji objašnjava celu igru bez reči:
**dva lifta jedan pored drugog, jedan osvetljen, jedan mračan.** Nikakav
tekst ne ide preko prve 3 sekunde; tekst dolazi tek na 0:03, kada je slika
već „uhvatila" oko.

### 0.3 Nema spojlera kraja

Šest krajeva se **ne prikazuje i ne imenuje** — ni u kadru, ni u tekstualnoj
kartici. Ne snimaj nijednu ending mini-scenu, ending widget, terminal ni
Return-to-Main-Menu ekran. Zatvaranje trailera je jedan dvosmislen mračan
kadar telefona snimljen u **normalnom gameplayu**, ne u ending sekvenci.

---

## 1. Priprema pred snimanje

### 1.1 Koji build

Snimaj iz builda **pokrenutog iz Steam Library-ja**, ne iz Explorera.
Shipping EXE pokrenut ručno ne dobija Steam ticket, pa AI chat ne radi
(`RELEASE_CHECKLIST.md` §5). Bez toga klipovi 6, 7 i 8 ne postoje.

Poslednji poznati validan setup: passwordovani `playtest` branch, BuildID
`24782464`, instaliran kroz Steam klijent.

### 1.2 Backend mora biti topao

Pre snimanja chat klipova otvori u browseru:

```
https://loop9-backend.onrender.com/readyz
```

Mora da vrati `{"status":"ready"}`. Ako je Render na free planu i bio je
neaktivan, prvi zahtev budi instancu i chat ume da visi na `Thinking…` par
desetina sekundi — to je neupotrebljivo u klipu od 10 s. Zagrej backend,
odigraj jedan probni chat koji **ne snimaš**, pa tek onda pusti OBS.

Ako svaki odgovor izgleda isto i generički, to nije model — to je moderation
fallback (`RELEASE_CHECKLIST.md` §4: nevalidan `AI_MODERATION_API_KEY` vraća
HTTP 200 i uvek istu in-fiction rečenicu). Takav klip se briše, ne montira.

### 1.3 Na kojoj petlji snimati chat

Chat klipove snimaj na **petlji 4 ili kasnijoj**. Backend je konfigurisan sa
dva tier-a: `cheap` model otvara petlje 1–3, `best` model petlje 4+
(`RELEASE_CHECKLIST.md` §4). Odgovor sa petlje 6 zvuči osetno bolje od
odgovora sa petlje 2, a trailer pokazuje samo jedan odgovor.

### 1.4 Jezik i UI

- Jezik igre: **English** (Settings → Language). Store je globalan, a kartice
  su na engleskom — mešani UI odmah izgleda kao amaterski snimak.
- Proveri isti jezik i u chatu i u interaction prompt-ovima, ne samo u meniju.
- U Steam klijentu privremeno **isključi achievement notifikacije**
  (Steam → Settings → Notifications). Tačan poziv lifta otključava
  `ACH_SPOT_*`, i Steam toast ume da uleti u sredinu najboljeg kadra.

### 1.5 Debug higijena

- Zatvori konzolu pre svakog snimanja. Nijedan frame ne sme da sadrži
  `AnomalyForce`, `AnomalyList`, `AnomalyHelp` ni njihov ispis.
- `stat none` — bez `stat unit`, `stat fps`, `stat game`, `stat gpu`.
- Bez Unreal Insights overlay-a, bez editor viewport-a, bez PIE.
- Ako koristiš Development build da bi forsirao anomalije, taj build
  **isto mora da se pokrene iz Steama** da bi chat radio. Debug komande su
  compiled-out iz Shipping-a, pa Shipping build znači da anomalije čekaš
  prirodno kroz petlje.

### 1.6 OBS podešavanja

| Stavka | Vrednost |
|---|---|
| Base / Output resolution | `1920×1080` (ili `2560×1440` → downscale na 1080) |
| FPS | **60** |
| Encoder | NVENC H.264 (ili x264 `veryfast`) |
| Rate control / bitrate | CBR, **40–50 Mbps** |
| Keyframe interval | 2 s |
| Format | MP4 (ili MKV pa remux) |
| Capture mode | **Game Capture**, ne Display Capture |
| Capture Cursor | **isključeno** |
| Webcam / mikrofon | nema izvora u sceni |
| Overlay-i (Discord, MSI AB, RTSS, GeForce) | ugašeni |
| Stream Deck / notifikacije | ugašene |

Sceni u OBS-u dodaj **samo** Game Capture izvor. Bez slika, bez teksta, bez
alert box-a, bez browser source-a.

### 1.7 Kako se snima

- Pokreni snimanje, **broji 2 sekunde mirno**, pa tek onda kreni da radiš
  radnju iz kadra, i posle radnje ostani mirno još 2 sekunde. Tako uvek imaš
  čist rez sa obe strane.
- Miš pomeraj **sporo i kratko**. Brzo trzanje kamerom je najčešći razlog
  zašto indie trailer izgleda loše.
- Za svaki kadar snimi **dva pokušaja**. Jeftinije je nego vraćati se sutra.
- Imenuj fajlove odmah: `clip01_elevators.mp4`, `clip02_baseline.mp4`, …

---

## 2. Shot table

Svaki red je jedan snimak. „Trajanje" je koliko ostaje **u traileru**;
snimaj duže i seci.

| # | Trajanje | Šta radiš u igri | Šta gledalac treba da razume | Kartica |
|---|---|---|---|---|
| **1** | 4 s | Stani u hodniku ispred oba lifta, tako da **oba stanu u kadar** — osvetljeni i mračni. Polako korači 2–3 koraka napred i stani. Bez okretanja kamere. | Ovo je igra o izboru između dva lifta. Čita se odmah, bez zvuka i bez teksta. | A (od 0:03) |
| **2** | 5 s | Petlja 1 (čist sprat). Stani na fiksnu tačku u open-space delu kancelarije i uradi **jedan spor pan udesno** preko stolova. Zapamti tačno gde stojiš. | Ovako izgleda normalno. | A (do 0:05) |
| **3** | 4 s | Kasnija petlja sa aktivnom **Move** anomalijom. Vrati se na **istu tačku iz Klipa 2**, isti pan, ista brzina. Okrenuta stolica / pomeren predmet mora biti u kadru. | Ista prostorija, nešto nije isto. Rez 2→3 je cela poenta igre. | B |
| **4** | 4 s | **Hide** anomalija. Priđi stolu/polici gde predmet fali, zastani, lagano se nagni ka mestu gde bi trebalo da bude. | Predmeti nestaju. Igra se igra pamćenjem. | — |
| **5** | 4 s | **Light** anomalija. Stani u hodniku sa plafonskim svetlom koje treperi, kadar odozdo-koso da se vidi i svetlo i hodnik. | Prostor je nestabilan. Atmosfera. | — |
| **6** | 9 s | Otvori telefon i **otkucaj poruku #1** (vidi §3). Sačekaj `Thinking…` i pusti da se odgovor ispiše do kraja. Ne pomeraj miša dok se kuca. | Ovo nije dialogue tree. Neko stvarno odgovara. | C |
| **7** | 7 s | Isti chat, **poruka #2** (vidi §3). Odgovor mora da bude „u karakteru", ne servisna informacija. | Dragojlo je lik, ne alat. | — |
| **8** | 6 s | **PhantomMessage** anomalija. Otvori chat i skroluj/zastani na poruci **koju nikad nisi poslao**. Drži kadar mirno da se pročita. | Anomalija ume da uđe u tvoj chat. Najjezivija ideja u igri. | — |
| **9** | 6 s | **Pursuer** anomalija. Okreni se u hodniku, uhvati figuru u kadru i **koračaj unazad** dok gledaš u nju. Ne trči, ne panič-vrti kameru. | Nešto se kreće po spratu. Ovo je horor, ne samo zagonetka. | — |
| **10** | 3 s | **Text / MaterialSwap** anomalija na magazinu. Priđi blizu i kadriraj **tesno samo na reč** (`RUN` / `HELP` / `DIE`). | Sprat ti se obraća. | — |
| **11** | 7 s | Uđi u kabinu **osvetljenog** lifta, pritisni dugme, ostani miran. Snimi ceo prelaz: zvuk dugmeta, zatvaranje vrata, fade u crno (~1.25 s). | Ovo je odluka. Pritisneš i nema nazad. | D |
| **12** | 3 s | Mračan deo sprata u normalnom gameplayu. Kadar na telefon na stolu. Bez interakcije, bez chata, bez kretanja. **Nije ending scena.** | Neko i dalje čeka da se javiš. Dvosmisleno, bez spojlera. | E |
| — | 6 s | Nije snimak: statična kartica sa logom (vidi §5). | Ime igre, „Coming Soon", wishlist. | F |

Šest različitih tipova anomalija (Move, Hide, Light, PhantomMessage, Pursuer,
Text) — **nijedan tip se ne ponavlja**. Ostala tri (Audio, DoorLock, Scale)
namerno ostaju za igru; Audio se ionako gubi u mutiranom autoplayu, a
DoorLock i Scale traže kontekst koji trailer nema vremena da postavi.

---

## 3. Chat poruke (tačan tekst za kucanje)

Kucaj **na engleskom**, jer je UI na engleskom. Poruke su kratke namerno —
dugačka poruka se ne pročita u klipu od 9 sekundi, a i sporije se kuca u kadru.

**Poruka #1 (Klip 6):**

```
Something changed up here. Where do I look?
```

**Poruka #2 (Klip 7):**

```
Why are you still here?
```

**Rezerva (ako neki odgovor ispadne mlak):**

```
Are you lying to me?
```

Napomene:

- Poruka #1 prodaje mehaniku: on odgovara na tvoje reči, ne bira iz menija.
  **Ali** — sposobnost da uputi igrača u pravi deo sprata zavisi od toga da
  li su `AnomalyZone` / `AnomalyObjectKind` popunjeni na toj anomaly
  komponenti (`docs/ANOMALIES.md` → AI context tagging). Ta polja su u
  `RELEASE_CHECKLIST.md` §9 još `[~]`. Ako nisu popunjena, on će pošteno
  reći da ne zna gde je — što je dizajnerski ispravno, ali u traileru zvuči
  kao da AI ne radi. Zato: **popuni zonu bar na jednoj anomaliji koju
  koristiš za snimanje**, ili preskoči „Where do I look?" i snimi samo
  „Something changed up here."
- Poruka #2 prodaje lika. Nju ne diraj — odgovor na to pitanje je najbolji
  argument da iza telefona stoji karakter, a ne FAQ.
- Backendu treba **internet i živa Steam sesija**. Ova tri klipa se ne mogu
  snimiti iz editora niti iz ručno pokrenutog EXE-a.
- Ako se u kadru pojavi `Still thinking...` duže od 2–3 s — snimaj ponovo.
  Čekanje je realno u igri, ali u traileru izgleda kao zamrznut build.

---

## 4. Redosled montaže i timeline

| Vreme | Klip | Beat |
|---|---|---|
| 0:00–0:04 | 1 | **Tihi hook.** Dva lifta. Bez teksta do 0:03. |
| 0:04–0:09 | 2 | Baseline: kako izgleda normalno. |
| 0:09–0:13 | 3 | **Reveal mehanike.** Isti kadar, promenjen detalj. Tvrd rez, bez fade-a. |
| 0:13–0:17 | 4 | Eskalacija 1 — nestao predmet. |
| 0:17–0:21 | 5 | Eskalacija 2 — svetlo. |
| 0:21–0:30 | 6 | **Hook #2: telefon.** Kucanje + živ odgovor. |
| 0:30–0:37 | 7 | Karakter, ne alat. |
| 0:37–0:43 | 8 | Poruka koju nisi poslao — spoj chata i anomalije. |
| 0:43–0:49 | 9 | **Vrhunac.** Pursuer. |
| 0:49–0:52 | 10 | Kratak udarac: `RUN` na magazinu. |
| 0:52–0:59 | 11 | **Payoff.** Pritisak dugmeta, vrata, fade u crno. |
| 0:59–1:02 | 12 | Mračan telefon. Dvosmisleno. |
| 1:02–1:08 | — | Logo + `COMING SOON`. |

**Ukupno: 68 s.**

Pravila reza:

- Rezovi 1→2, 2→3, 3→4, 4→5 su **tvrdi** (bez prelaza). Fade između njih
  ubija „spot the difference" efekat.
- Jedini fade-ovi: `fade from black` 0.3 s na samom početku, i `fade to black`
  koji **već postoji u igri** na kraju Klipa 11 (ne dodaj svoj preko njega,
  dobićeš duplo tamnjenje).
- Klip 12 ulazi iz crnog i izlazi u crno.
- Logo kartica ulazi iz crnog na 1:02 i drži do kraja.

Muzika:

- Isključivo licencirano: **CapCut audio library**, **YouTube Audio Library**
  ili **Epidemic Sound**. Nijedan komercijalni/copyright track — Steam trailer
  ide i na YouTube kanal igre, a claim na traileru je gubljenje nedelje.
- Traži: nizak drone / dark ambient bez ritma, bez vokala, bez „epic trailer
  braaam" udara.
- Struktura: tiho 0:00–0:21, pojačaj oko 0:43 (Pursuer), **potpuna tišina od
  0:59** do kraja. Tišina na kraju radi bolje od bilo kakvog udara.
- Zadrži originalni zvuk igre ispod muzike (~-8 dB) za klipove 5, 9 i 11 —
  zvuk dugmeta lifta i vrata su najbolji zvuk koji imaš.

Export: `1920×1080`, `60 fps`, H.264 MP4, ~20 Mbps. Za Steam thumbnail izaberi
frame iz Klipa 1 (dva lifta), ne logo.

---

## 5. Tekstualne kartice — tačan engleski tekst

Sve kartice su **beli sans-serif tekst preko snimka** (osim F), donja trećina,
0.3 s fade-in / fade-out, bez ijednog CapCut animacionog preseta. Bez senki,
bez outline-a, bez emojija, bez velikih blokova teksta.

| ID | Vreme | Tekst |
|---|---|---|
| **A** | 0:03–0:05 | `THE SECOND FLOOR REPEATS.` |
| **B** | 0:10–0:12 | `SPOT WHAT CHANGED.` |
| **C** | 0:22–0:27 | `HE ANSWERS IN REAL TIME.`<br>`No dialogue trees.` |
| **D** | 0:53–0:58 | `LIT ELEVATOR: SOMETHING CHANGED.`<br>`DARK ELEVATOR: NOTHING DID.` |
| **E** | 0:59–1:02 | `CHOOSE WRONG. START OVER.` |
| **F** | 1:02–1:08 | logo (slika) + `COMING SOON` + `Wishlist on Steam` |

Kartica D namerno dolazi tek na kraju, uz stvarni pritisak dugmeta — pravilo
se pamti kada ga vidiš primenjeno, a ne kada ga pročitaš na 5. sekundi.

Sav tekst je izveden iz postojećeg store copy-ja (`STORE_PAGE.md`): „The second
floor loops", „Spot what changed", „he answers in character, in real time",
„THE LIT ELEVATOR / THE DARK ELEVATOR", „Choose wrong, and you start over".
Nemoj izmišljati nove slogane — konzistentnost sa store stranicom je deo utiska
da je igra gotova.

**Finalni frame (F):**

- Pozadina: čisto crna.
- Logo: **postojeći asset**, ne pravi novi:
  `Marketing/Steam/StoreUpload/library_logo_transparent_1280x720.png`
- Logo je 1280×720. Na 1920×1080 platnu stavi ga na ~66% širine kadra i
  **nemoj ga razvlačiti na pun frame** — upscale će se videti.
- Ispod loga: `COMING SOON`, pa manjim slovima `Wishlist on Steam`.
- Bez datuma. Release datum još nije fiksiran (`STORE_PAGE.md` → „Coming soon").

---

## 6. Greške koje ubijaju trailer

- **Dugo hodanje bez događaja.** Nijedan kadar u kome se 5 s samo hoda kroz
  prazan hodnik. Ako se u kadru ništa ne menja, kadar traje 3 s ili ide napolje.
- **Ista anomalija dva puta.** Šest kadrova = šest različitih tipova. Dva
  Move kadra izgledaju kao da igra ima jednu anomaliju.
- **Čitljiv placeholder tekst.** Bilo koji `Lorem`, `TODO`, `TEXT_`,
  `SM_Lamp_03`, neprevedeni ključ ili prazan string u kadru = ponovo snimaj.
- **Magazin u širokom kadru.** Baseline korica sadrži tekst
  `PC / THE YEAR 2000`, a igra se dešava 2003. — u traileru to izgleda kao
  bug ili nedovršen asset. Klip 10 je **tesan kadar samo na reč** koja se
  pojavljuje (`RUN` / `HELP` / `DIE`), korica ne sme u kadar.
- **Mešan jezik UI-ja.** Srpski chat + engleske kartice = amaterski snimak.
  Sve na engleskom, uključujući interaction prompt-ove.
- **Spojlovan kraj.** Nijedna ending mini-scena, ending widget, terminal,
  ime kraja ni „Return to Main Menu" ekran. Ni u jednom frame-u.
- **Snimak gde je AI pao.** Ako je odgovor generička in-fiction fallback
  rečenica (moderation/provider fallback), taj klip se briše. Trailer koji
  reklamira živ AI, a pokazuje fallback, radi protiv sebe.
- **Debug ostaci.** Konzola, `stat` overlay, FPS brojač, kursor miša,
  Steam achievement toast, Discord popup.
- **Trzava kamera.** Brzo vrtenje mišem u horor kadru poništava atmosferu.
- **Fade preko fade-a** na Klipu 11 — igra već radi svoj fade u crno.
- **Predugačke poruke u chatu.** Ako se ne pročita za 4 s, ne ide u trailer.

---

## 7. Checklist za jedno veče

**Priprema (15 min)**

- [ ] `/readyz` vraća `{"status":"ready"}`.
- [ ] Igra pokrenuta **iz Steam Library-ja**.
- [ ] Jezik igre = **English**, proveren i u chatu.
- [ ] Steam achievement notifikacije isključene.
- [ ] Svi overlay-i (Discord / GeForce / MSI AB / RTSS) ugašeni.
- [ ] OBS: 1080p60, CBR 40–50 Mbps, Game Capture, kursor isključen, bez webcama.
- [ ] Konzola zatvorena, `stat none`.
- [ ] Odigran jedan probni chat koji se **ne snima** (grejanje backenda).

**Snimanje (~90 min)**

- [ ] Klip 1 — dva lifta u istom kadru.
- [ ] Klip 2 — baseline pan, petlja 1. **Zapamti tačku i pravac.**
- [ ] Klip 3 — isti pan, Move anomalija aktivna.
- [ ] Klip 4 — Hide anomalija.
- [ ] Klip 5 — Light anomalija.
- [ ] Klip 6 — chat, poruka #1, petlja 4+.
- [ ] Klip 7 — chat, poruka #2.
- [ ] Klip 8 — PhantomMessage u chat logu.
- [ ] Klip 9 — Pursuer, hodanje unazad.
- [ ] Klip 10 — magazin, tesan kadar na reč.
- [ ] Klip 11 — pritisak dugmeta osvetljenog lifta + fade.
- [ ] Klip 12 — mračan kadar telefona, van ending scene.
- [ ] Svaki klip snimljen **dva puta**.

**Montaža (~60 min)**

- [ ] Klipovi ubačeni redom 1–12 i isečeni na trajanja iz §2.
- [ ] Rezovi 1→5 tvrdi, bez prelaza.
- [ ] Kartice A–E dodate na tačna vremena iz §5.
- [ ] Logo frame F sa postojećim `library_logo_transparent_1280x720.png`.
- [ ] Licencirana muzika dodata; tišina od 0:59.
- [ ] Gameplay zvuk zadržan na klipovima 5, 9, 11.
- [ ] Ukupno trajanje između **45 i 75 s** (cilj 68 s).

**Kontrola pre uploada**

- [ ] Odgledan **mutiran**, od početka — da li prve 3 sekunde nose?
- [ ] Odgledan na telefonu — da li se sve kartice pročitaju?
- [ ] Pauziran na svakih 5 s: nigde konzole, `stat`-a, kursora, toast-a,
      placeholder teksta, srpskog UI-ja, korice magazina.
- [ ] Nigde nijedan kadar iz ending sekvence.
- [ ] Nijedan kadar nije iz editora / PIE-a.
- [ ] Export 1080p60 H.264, thumbnail = frame iz Klipa 1.
- [x] Upload u Steamworks → Store Presence → Trailers; zatim **Publish**,
      ne samo Save (`RELEASE_CHECKLIST.md` §2). Valve review od ~22.08.2026.
- [x] Štiklirati „Snimiti i montirati gameplay trailer" u
      `RELEASE_CHECKLIST.md`, sekcija **Store grafika**.
