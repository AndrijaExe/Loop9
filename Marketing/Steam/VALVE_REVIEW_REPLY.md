# Valve build review — failed BuildID 24910264 (28.08.2026.)

Valve je oborio build iz tri razloga i uz to blokirao prodaju u Kini. Kina je
informacija, ne zadatak. Ostala tri se rešavaju: dva čisto u Steamworksu, treći
tako što recenzentu damo Development build i uputstvo.

**Redosled:** prvo Steamworks (§1, §2), pa Development build na passworded granu
(§3), pa odgovor na tiket (§4). Bez §3 recenzent ne može do endinga jer su sve
debug komande isečene iz Shipping builda.

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

Prođi ove četiri stavke **tim redom**. Prva koja ne štima je uzrok.

### 2.1 Proveri kvotu — najčešći uzrok baš ovog simptoma

Steamworks → App Admin → **Cloud**
(`https://partner.steamgames.com/apps/cloud/4982260`)

Ako su **Byte quota** ili **Number of files** ostali na `0`, Steam Cloud je
mrtav bez obzira na Auto-Cloud pravila, a kategorija „Steam Cloud" i dalje stoji
na store strani. To se poklapa sa „syncing appears to be enabled" + „no sync
data". Postavi nešto komotno, npr. `1048576` bajtova i `20` fajlova.

### 2.2 Proveri da putanja stvarno postoji na disku

Pre nego što veruješ podešavanju, pokreni **Shipping** build i pogledaj da li
postoji:

```
%LOCALAPPDATA%\Loop9\Saved\Config\Windows\Game.ini
```

Zalepi to u Explorer adresnu liniju. Ako fajla nema tu, Auto-Cloud pravilo gađa
prazno i sve ostalo je nebitno — nađi gde je fajl stvarno završio (najverovatnije
`...\steamapps\common\Loop 9\Loop9\Saved\Config\Windows\Game.ini`) i podesi
pravilo na to.

Zašto je ovo realan rizik: UE u Shipping buildu Saved folder preusmerava u
`%LOCALAPPDATA%`, ali u Development buildu ga ostavlja pored `.exe`. Znači
**Development build iz §3 neće pisati na istu putanju kao Shipping.** Ako
recenzent testira Cloud na debug grani, past će opet. Zato debug build ide na
posebnu granu, a Cloud se verifikuje na `default` Shipping grani.

### 2.3 Proveri Auto-Cloud polja

Na istoj Cloud strani, pod *Auto-Cloud*, tri polja su odvojena i lako se
pobrkaju. Treba:

| Polje | Vrednost |
|---|---|
| Root Path | `WinAppDataLocal` |
| Subdirectory | `Loop9/Saved/Config/Windows` |
| Pattern | `Game.ini` |

Česta greška je nabijanje cele putanje u *Subdirectory* i ostavljanje `*` u
*Pattern*. Ako nisi siguran gde fajl završava, dodaj i drugo pravilo sa Root
Path `gameinstall` i istim subdirectory-jem — pokriva oba slučaja i ne smeta.

Dodaj i drugo pravilo za `GameUserSettings.ini` (isti subdirectory). Nije bilo u
planu, ali recenzent eksplicitno testira „save data doesn't sync between PCs", a
grafika i jezik su vidljiviji dokaz sinhronizacije od jednog reda u `Game.ini`.

### 2.4 Publish

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
   `Builds/v1.0.0/Windows`.
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
- Vrati `contentroot` u `app_build_4982260.vdf` na `Builds/v1.0.0/Windows` ako
  već nije.
