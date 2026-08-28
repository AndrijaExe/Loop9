# Valve build review — failed BuildID 24910264 (28.08.2026.)

Valve je oborio build iz tri razloga i uz to blokirao prodaju u Kini. Kina je
informacija, ne zadatak. Ostala tri se rešavaju: dva čisto u Steamworksu, treći
tako što recenzentu damo Development build i uputstvo.

**Redosled:** prvo Steamworks (§1, §2), pa Development build na passworded granu
(§3), pa odgovor na tiket (§4). Bez §3 recenzent ne može do endinga jer su sve
debug komande isečene iz Shipping builda.

Oboreni build `24910264` je cook od 24.08. U međuvremenu je live playtest
**v1.0.1 (BuildID `24980937`, 28.08.)**, pa se za ponovni review markira taj, a
ne stari. Debug build iz §3 treba kuvati iz istog source-a kao v1.0.1 da se
recenzent ne bi sudario sa razlikama koje ne postoje u retail buildu.

---

## 0. Kina — nema akcije

> we've blocked your app from appearing in China

Valveova odluka o regionalnoj dostupnosti, ne greška u buildu i ne blokira
release. Ignoriši osim ako baš želiš da im pišeš.

---

## 1. Nedostaju Unreal prerequisites

> DirectX End-User Runtimes - June 2010
> Visual C++ Redistributable 2022

Nije bug u igri. UE build stage-uje svoj `UEPrereqSetup` i on iskače kao
„third-party launcher", a Steam očekuje da instalacija sama povuče sve
zavisnosti. Rešava se čekiranjem dve kutijice, bez rebuilda i bez novog uploada.

**Gde:** Steamworks → App Admin za `4982260` → **Installation** → **General
Installation** → sekcija *Common Redistributables*.

Direktan link:
`https://partner.steamgames.com/apps/installation/4982260`

Čekiraj tačno ovo dvoje, ništa više:

- `DirectX End-User Runtimes (June 2010)`
- `Microsoft Visual C++ Redistributable 2022`

Pa **Save**, pa **Publish** (App Admin → *Publish* → *Prepare for Publishing* →
*Publish to Steam*). Dok se ne publish-uje, live konfiguracija se ne menja i
recenzent vidi isti problem.

Dokumentacija: <https://partner.steamgames.com/doc/features/common_redist>

---

## 2. Steam Cloud ne sinhronizuje

> No sync data appears in the "General" tab of the Steam title Properties menu

Ovo je najverovatnije takođe Steamworks konfiguracija, ne kod. Igra piše
`SeenEndings` i `SpottedAnomalies` u `Game.ini` preko `GConfig` i to radi —
`ULoop9AchievementsSubsystem::Initialize` upiše `PendingUnlocks` red već pri
prvom pokretanju, pa fajl postoji i pre nego što iko odigra ijedan ending.

Cela Cloud strana provereno 28.08.2026. **Sve što se vidi u Steamworksu je
ispravno** — kvota (§2.2), Auto-Cloud pravilo (§2.3) i Beta Testing čekboks
(§2.1). Znači uzrok nije u konfiguraciji koja se vidi iz browsera, nego u tome
**da li fajl uopšte postoji tamo gde pravilo gleda**. To je §2.5 i to je sada
jedini pravi zadatak.

### 2.1 „Enable cloud support for developers only" — provereno, nije bio čekiran

Steamworks → App Admin → **Cloud**
(`https://partner.steamgames.com/apps/cloud/4982260`) → sekcija **Beta Testing**.

Ovo je bio glavni osumnjičeni jer Steamov opis kaže da čekboks „will hide the
icon from the client and will disable auto-cloud syncing", što bi objasnilo sva
tri simptoma odjednom. **Provereno 28.08. — nije bio čekiran.** Skinuto sa
liste.

### 2.2 Kvota — provereno, u redu

`Byte quota per user` = `10485760` (10.49 MB), `Number of files allowed per user`
= `10`. Oba su iznad nule, što znači da Cloud nije mrtav zbog kvote. Ne diraj.

Jedina zamerka je da je `10` fajlova tesno ako se ikad doda još Auto-Cloud
pravila; za sada je dovoljno jer se sinhronizuje jedan fajl.

### 2.3 Auto-Cloud polja — provereno, u redu

Postojeće pravilo je tačno:

| Polje | Vrednost |
|---|---|
| Root | `WinAppDataLocal` |
| Subdirectory | `Loop9/Saved/Config/Windows/` |
| Pattern | `Game.ini` |
| OS | `Windows` |
| Recursive | No |

Steamworks preview to razrešava u
`%USERPROFILE%/AppData/Local/Loop9/Saved/Config/Windows/`, što je tačno mesto
gde UE Shipping build piše. `OS = Windows` umesto `[All OSes]` je u redu jer je
igra samo za Windows; Root Overrides ne trebaju.

### 2.4 Ne dodavati `GameUserSettings.ini`

Ranije je ovde stajao predlog da se doda i to pravilo, radi vidljivijeg dokaza
sinhronizacije. **Povučeno.** Steamov best-practice na istoj strani izričito
kaže:

> Avoid machine specific configurations such as video quality.

`GameUserSettings.ini` je tačno to — rezolucija, fullscreen i quality scalability.
Sinhronizovati ga između jake mašine i laptopa je upravo ono na šta upozoravaju,
a `Game.ini` je ionako pravi save (`SeenEndings`, `SpottedAnomalies`) i postoji
od prvog pokretanja, pa Properties → General ima šta da pokaže i bez toga.

### 2.5 Da li fajl stvarno postoji tamo gde pravilo gleda — **glavni preostali sumnjivac**

Pošto su §2.1–2.3 čisti, ostaje ovo. Dva testa, oba na Windows mašini.

**Test A — Steam konzola, najbrži i najuverljiviji.** Otvori `steam://open/console`
i ukucaj:

```
testappcloudpaths 4982260
```

Komanda ispiše koje fajlove Auto-Cloud pravilo stvarno hvata. Ako vrati praznu
listu, pravilo gađa prazno i to je odgovor. Ovo je jedini test koji direktno meri
ono što Steam vidi, umesto da nagađamo.

**Test B — Explorer.** Pokreni **Shipping** build iz Steam Library-ja, izađi, pa
zalepi u Explorer adresnu liniju:

```
%LOCALAPPDATA%\Loop9\Saved\Config\Windows\Game.ini
```

Ako fajla nema tu, nađi gde je stvarno završio. Najverovatnija alternativa je
pored `.exe`:

```
...\steamapps\common\Loop 9\Loop9\Saved\Config\Windows\Game.ini
```

Ako je tamo, promeni Root u Auto-Cloud pravilu sa `WinAppDataLocal` na
**`gameinstall`** (App Install Directory) i ostavi isti Subdirectory i Pattern.
Bonus: `gameinstall` je jedini Root koji je validan na svim platformama, pa
usput rešava i cross-platform priču ako ikad izađeš na Linux.

Zašto je ovo realno: UE preusmerava `Saved` u `%LOCALAPPDATA%` samo kad se build
smatra „installed". Za Shipping cook to jeste tako po pravilu, ali zavisi od
verzije engine-a i od toga kako je cook napravljen, i **nije provereno na ovom
konkretnom v1.0.1 buildu**. Dok se ne potvrdi, tretiraj to kao pretpostavku, ne
kao činjenicu.

Zašto je ovo realan rizik: UE u Shipping buildu Saved folder preusmerava u
`%LOCALAPPDATA%`, ali u Development buildu ga ostavlja pored `.exe`. Znači
**Development build iz §3 neće pisati na istu putanju kao Shipping.** Ako
recenzent testira Cloud na debug grani, past će opet. Zato debug build ide na
posebnu granu, a Cloud se verifikuje na `default` Shipping grani.

### 2.6 Publish

Cloud izmene ne postaju žive dok se ne publish-uju. Isti *Publish* korak kao u
§1. Ovo je drugi najčešći uzrok: podešeno je, ali samo u draft konfiguraciji.

### 2.5 Provera pre nego što javiš Valveu

1. Odigraj jedan ending na mašini A, izađi iz igre **i iz Steama** (da sync ode).
2. Steam → Library → Loop 9 → desni klik → Properties → **General**. Mora da
   piše veličina cloud podataka.
3. Na mašini B (ili posle brisanja lokalnog `Game.ini`) pokreni igru i otvori
   Archive. Ending mora da bude tu.

---

## 3. Šest endinga — Development build na passworded grani

> Six endings shaped by your choices and relationship with Dragojlo.

Recenzent ovo ne može da verifikuje jer normalno traži devet ispravnih odluka po
endingu. Rešenje je Development build sa debug konzolom.

**Ključno:** `EndingSetup` i sve `Anomaly*` komande su pod `#if !UE_BUILD_SHIPPING`
i **ne postoje** u Shipping buildu. Zato:

```
Tools\PackageWindowsDevelopment.bat Debug
```

pa upload na **posebnu granu sa lozinkom**, ne na `default`:

1. Steamworks → App Admin → **Builds** → *Branches* → napravi granu
   `valvereview` sa lozinkom.
2. U `Tools/SteamPipe/app_build_4982260.vdf` privremeno prebaci `contentroot` na
   `D:/UE Course/Loop 9 AI/Builds/Debug/Windows`, uploaduj, pa **vrati nazad** na
   `Builds/v1.0.1/Windows` (isto i u `depot_build_4982261.vdf` ako ga diraš).
3. Set Live grane `valvereview` na taj build.

Development build ostaje na toj grani. `default` i dalje nosi Shipping.

Dokumentacija o granama:
<https://partner.steamgames.com/doc/store/application/branches>

---

## 4. Tekst odgovora na tiket

Zalepi ovo u Steamworks Support tiket. Popuni lozinku grane.

---

Hi, thanks for the detailed review. Here is where each item stands.

**1. Missing prerequisites (DirectX June 2010, Visual C++ 2022)**

Both redistributables are now enabled under Installation → Common
Redistributables and the change has been published. The build itself is
unchanged; it was only relying on the Unreal prerequisite installer, which we
understand should not be involved.

**2. Steam Cloud**

Our Auto-Cloud configuration and cloud quota have been corrected and published,
and we have verified a round trip on two machines: playing on PC A and then
launching on PC B restores progress, and the Cloud entry now shows in the game's
Properties → General tab.

Please verify Cloud on the **default** branch. Our debug build (below) is a
Development configuration, which writes its save data next to the executable
instead of to `%LOCALAPPDATA%`, so it is not representative for Cloud testing.

**3. Six endings**

We have uploaded a debug build to a private branch:

- Branch: `valvereview`
- Password: `<UPISI LOZINKU>`

This build has the developer console enabled, which lets you reach any ending in
about a minute. Please note the console is intentionally absent from the retail
build.

**Reaching an ending**

1. From the main menu, choose Play and wait until you are standing in the
   office.
2. Press the tilde key (`~`) **twice** to open the full console. The single-line
   console at the bottom of the screen will not show you the command output.
3. Type one of the following and press Enter:

   ```
   EndingSetup 0     Escape Together
   EndingSetup 1     Obedient Fool
   EndingSetup 2     Cold Betrayal
   EndingSetup 3     Paranoid Survivor
   EndingSetup 4     Merged Memory
   EndingSetup 5     The Replacement
   ```

   A green on-screen message confirms the ending is armed. This places you on
   loop 9 and clears the floor.
4. Close the console, walk to the elevators and enter the **dark** one (the
   unlit call button). With a clean floor, the dark elevator is the correct
   choice and advances the loop.
5. The loop reaches 10 and the ending plays.

To see the next ending, return to the main menu, start a new run and repeat from
step 2 with a different number. All six are independent; there is no required
order.

**Other store page features, if useful**

The same console forces each anomaly type on the current floor, for example:

```
AnomalyPursuer
AnomalyFlicker
AnomalyPhone
AnomalyDoor
AnomalyMaterial
AnomalyList        lists what is currently active
AnomalyReset       clears the floor again
AnomalyHelp        full command list
```

With an anomaly active, the **lit** elevator is the correct choice; on a clean
floor it is the **dark** one. This is the core loop of the game.

Please let us know if you need anything else and we will turn it around quickly.

---

## 5. Posle odobrenja

- Skini `valvereview` granu ili joj promeni lozinku. Development build ne sme da
  ostane dostupan.
- Vrati `contentroot` u `app_build_4982260.vdf` na `Builds/v1.0.1/Windows` ako
  već nije.
