# Valve build review — failed BuildID 24910264 (28.08.2026.)

Valve je oborio build iz tri razloga i uz to blokirao prodaju u Kini. Kina je
informacija, ne zadatak. Ostala tri se rešavaju: dva čisto u Steamworksu, treći
tako što recenzentu damo Development build i uputstvo.

**Redosled:** prvo Steamworks (§1, §2), pa Development build na passworded granu
(§3), pa odgovor na tiket (§4). Bez §3 recenzent ne može do endinga jer su sve
debug komande isečene iz Shipping builda.

Oboreni build `24910264` je cook od 24.08. Live playtest je
**v1.0.2 (BuildID `25008533`)**. Debug `valvereview` je **`24998416`**.

**Cloud je OK na ovoj mašini (29.08. 12:52).** Properties → General:
**108 bytes stored.** Fajl je
`userdata\375407870\4982260\remote\Game.ini` (`CloudReady=1`). Auto-Cloud
i dalje briše AppData kopiju; Valve gleda Properties, to je sada tu.
Redistributables + endings + Cloud mogu u tiket. `default` ručni Set Live.
Ne tvrdi two-machine round-trip dok to ne uradiš.

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

**Uzrok je kod, ne Steamworks.** Putanja i Auto-Cloud pravilo su tačni.
Shipping build piše u `%LOCALAPPDATA%\Loop9\Saved\Config\Windows\`. Posle
Play iz Library tu stoje `GameUserSettings.ini` i `steam_autocloud.vdf`
(accountid `375407870`) — Steam **jeste** gledao taj folder. `Game.ini`
**nije postojao** ni posle 17:45, ni posle 19:39, **ni posle Play 20:23 na
`24998396`**. Steamworks nije uzrok.

Tri baga, redom:

1. `Initialize` zove `PersistPendingUnlocks()` odmah, ali prazan `SetString`
   za `PendingUnlocks`/`SeenEndings` **ne kreira fajl**. Recenzent koji uđe u
   kancelariju i izađe nema šta da sinhronizuje.
2. Prvi `CloudReady=1` flush (u `24997951`) ide na Unrealov `GGameIni`, što u
   packaged Shipping **nije** Auto-Cloud putanja.
3. `24998396` je flush-ovao pravu putanju, ali UE 5.8 `GConfig->SetString`
   zove `Find()`: ako `Game.ini` **ne postoji**, vraća null i **ne radi
   ništa**. Zato fajl i dalje nije nastao.

Fix: `EnsureCloudSaveFile()` piše
`%LOCALAPPDATA%\Loop9\Saved\Config\Windows\Game.ini` preko `FFileHelper`,
bez GConfig keša. To je u **`25008390`** na `playtest`.

`GameUserSettings.ini` namerno nije na Cloud-u (mašinski settings).

`default` i dalje mora ručno Set Live u Steamworks Builds — steamcmd ne sme
da postavi default granu.

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
Sinhronizovati ga između jake mašine i laptopa je upravo ono na šta upozoravaju.
Ne dodavaj ga na Auto-Cloud da se „prođe" review.

### 2.5 Test A/B — putanja OK, fajl još nije

Putanja je tačna. Shipping iz Steam Library-ja piše u
`%LOCALAPPDATA%\Loop9\Saved\Config\Windows\`. `steam_autocloud.vdf` postoji.
`Game.ini` **nije** nastao na `24997951`. Live cook za proveru je
**`24998396`**. Fajl nije pored `.exe`. Root ostaje `WinAppDataLocal`. Ne
prebacuj na `gameinstall`.

Opcioni sanity check u Steam konzoli posle Play + Exit:
`testappcloudpaths 4982260` — treba da vidi `Game.ini` kad fajl postoji.

UE Development cook piše Saved pored `.exe`, Shipping u `%LOCALAPPDATA%`.
Zato debug grana **nije** Cloud test. Cloud se verifikuje na `default` /
`playtest` Shipping.

### 2.6 Provera pre nego što javiš Valveu

1. Shipping **`25008533`** je na `playtest`. Cloud na ovoj mašini: **108
   bytes** u Properties → General (29.08. 12:52). Set Live ručno na **default**.
2. Two-machine round-trip ostaje opcioni. Ne piši ga u tiket dok ga ne uradiš.

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
   `valvereview` sa lozinkom. **Lozinku ne commit-uj.**
2. Upload Development cook-a preko `Tools/SteamPipe/UploadValvereview.bat`
   (`app_build_valvereview.vdf`). Playtest VDF ostaje na `v1.0.2`.
3. Set Live grane `valvereview` na taj build (VDF već ima `setlive valvereview`).

Development build ostaje na toj grani. `default` i dalje nosi Shipping.

Dokumentacija o granama:
<https://partner.steamgames.com/doc/store/application/branches>

---

## 4. Tekst odgovora na tiket

Zalepi ovo u Steamworks Support tiket. Popuni lozinku grane.

**Cloud paragraf:** Properties na ovoj mašini pokazuje 108 bytes na
`25008533`. Two-machine round-trip **nemoj** da tvrdiš. Markirati
**`25008533`**, ne starije BuildID-ove.

---

Hi, thanks for the detailed review. Here is where each item stands.

**1. Missing prerequisites (DirectX June 2010, Visual C++ 2022)**

Both redistributables are now enabled under Installation → Common
Redistributables and the change has been published. The build itself is
unchanged; it was only relying on the Unreal prerequisite installer, which we
understand should not be involved.

**2. Steam Cloud**

The Shipping build now writes a small progress file through the Steam Cloud
API on first launch. After Play + Exit, the game's Properties → General tab
shows Cloud usage (108 bytes in our test: `CloudReady=1` plus persist keys).
We do not sync `GameUserSettings.ini` (machine-specific video settings).

Please verify Cloud on the **default** branch. Our debug build (below) is a
Development configuration and is not representative for Cloud testing.

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
AnomalyHide
AnomalyMove
AnomalyDoor
AnomalyMaterial
AnomalyText
AnomalyScale
AnomalyPhantom
AnomalyLoopNumber
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
- Vrati `contentroot` u `app_build_4982260.vdf` na `Builds/v1.0.2/Windows` ako
  već nije.
