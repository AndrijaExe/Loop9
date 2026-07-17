# Editor TODO — šta ručno odraditi u Unreal editoru

Živi dokument: sve što je urađeno u C++/config-u, a traži ručni korak u
editoru (ili Steamworks-u) da bi proradilo. Kad nešto završiš, štrikliraj.

---

## Status (15.07.) — gameplay / settings / i18n / telemetry GOTOVO

Čeka se: **Steam App ID** (identity verification). Do tada nema obaveznog
editor/C++ posla za release blokere.

**Novo (15.07. popodne, traži recompile):** fix za combo vrednosti koje se ne
osveže odmah posle promene jezika iz pause menija — refresh je išao preko world
timera koji ne otkucava dok je igra pauzirana; sada ide preko `FTSTicker`.
Smoke: pauza → settings → promeni jezik → combo vrednosti (Windowed/Quality/
Uncapped) se odmah prevedu; Back/Resume ne ostavljaju overlay.

---

## 0–2. Setup / Settings / Audio — GOTOVO

(vidi istoriju u gitu; settings, Ambient, jezici, overlay cleanup — smoke OK)

## 3. Lokalizacija — GOTOVO (+ opciono održavanje)

- [x] en/sr/de/fr/ru locres + settings/meni smoke (uključujući restart)
- [ ] Kad dodaš novi NSLOCTEXT: GatherText → PO → GatherText
- [x] Prazni `msgstr` u PO dopunjeni (15.07.): svi C++ stringovi za de/fr/ru
  (chat, endinzi, terminal, tips, anomalije, interakcije) + vidljivi asset
  stringovi za sve jezike ("THANKS FOR PLAYING!", "Return to Main Menu",
  "Talk on phone...", "Send"...). Preostali prazni su BP defaulti koje C++
  pregazi u runtime-u (PLAY/SETTINGS/Master Volume...) i editor-interno.
- [ ] **Pokrenuti GatherText ponovo** da se novi prevodi kompajluju u `.locres`
  (PO je izmenjen, locres u repou je još stari) + smoke de/fr/ru.

## 4. Anomalije

- [x] Scale + Phantom u nivou, smoke OK
- [~] Clock — kod ostaje, nije u nivou (namerno)
- [ ] Steam toast za `ACH_SPOT_SCALE` / `ACH_SPOT_PHANTOM` — tek sa pravim App ID

## 4b. NOVO (15.07., redizajn 16.07.): Item inspection (Resident Evil stil)

Novi feature u C++: pokupiš predmet pogledom + E, igra se pauzira, kamera se
prebaci u **crnu sobu** (izolovana scena visoko iznad mape — pozadina potpuno
crna, predmet oštar i osvetljen key+fill svetlom) i rotiraš ga mišem (ili
desnim stikom). Izlaz: **Esc / E / desni klik / B ili X na gamepadu**.

Klase: `UInspectableComponent` (dodaj na bilo koji actor + pozovi
`StartInspection`), `AInspectableItem` (gotov actor), `AInspectionStageActor`
(interni, ne diraš ga).

U editoru:
- [ ] Recompile (novi fajlovi u `Source/Loop9/Interaction/`).
- [ ] U nivo prevuci **InspectableItem** (Place Actors → traži "Inspectable
  Item") ili napravi BP child. Dodeli `Mesh` (StaticMesh) — npr. fotografija,
  šolja, bedž, dokument.
- [ ] Po želji podesi na `Inspectable` komponenti: `InitialRotationOffset`
  (koja strana gleda ka igraču na otvaranju), `RotationSpeed`, `TargetRadiusCm`
  (koliko krupno se prikazuje), `DisplayName` (za budući caption).
- [ ] Za postojeće BP actore: dodaj `InspectableComponent` i iz BP-a pozovi
  `StartInspection` (npr. iz `TryInteract`); event dispecheri
  `OnInspectionStarted/Ended` postoje za lore/anomaly logiku.
- [ ] GatherText (dodat je novi string `Loop9Interaction,Inspect` = "Examine",
  prevodi već upisani u PO za svih 5 jezika).
- [ ] Smoke: E na predmet → pauza + crna soba + rotacija mišem; Esc vraća
  igru i kameru; pause meni ne može da se otvori dok traje inspekcija.

### 4c (16.07.): MaterialSwapAnomalyComponent — "pogrešne novine"

Nova anomalija: mesh na actoru dobije drugi materijal dok je anomalija aktivna
(novine sa drugim naslovom, poster sa pogrešnim slovima...). **Računa se kao
postojeći Text tip** — ništa se ne menja u backendu/achievementima.

- [ ] Drugi AI generiše 2–3 varijante tekstura novina → napravi materijale
  (npr. `MI_Newspaper_Anomaly1/2/3` — kopija normalnog materijala sa
  zamenjenom teksturom).
- [ ] Na actor novina dodaj `MaterialSwapAnomalyComponent`:
  - `AnomalyMaterials` = anomalne varijante (bira nasumičnu pri aktivaciji),
  - `MaterialSlot` = slot novinskog materijala (obično 0),
  - `TargetComponentName` ostavi None (uzima prvi mesh) ili upiši ime
    komponente ako actor ima više mesheva.
- [ ] Kombinacija sa inspekcijom: inspekcija kopira TRENUTNO primenjene
  materijale — kad je anomalija aktivna, igrač u ruci vidi "pogrešne" novine.
  (Napomena: to važi kad se mesh čita sa actora; ako koristiš `MeshOverride`
  na `InspectableComponent`, materijali dolaze iz override mesha.)
- [ ] Smoke: forsiraj anomaliju → novine promenjene → sledeći loop vraćene.

### 4d (16.07.): InspectableComponent je sada drop-in

Više NE moraš da koristiš `InspectableItem` actor: dodaj `InspectableComponent`
na **bilo koji actor** u sceni (Details → Add → Inspectable) i actor odmah
dobija "Examine" prompt + inspekciju (E). `PromptText` na komponenti menja
tekst prompta po actoru. `InspectableItem` i dalje radi kao i do sad.

Ako actor ima više static mesh komponenti, postavi `TargetMeshComponentName`.
Ako koristiš `MeshOverride`, `bCopyOwnerMaterialsWithMeshOverride` čuva trenutne
materijale sa actora (uključujući anomaliju). Za translucent/dither materijale
popuni `InspectionMaterialOverrides` neprozirnim inspection varijantama.

### 4b-crna-soba (16.07. popodne): REDIZAJN — nema više blura

Blur pristup (DOF pa stencil maska) je izbačen: predmet je ispadao providan
i mutan. Novo rešenje je **crna soba**: `AInspectionStageActor` se teleportuje
300 m iznad igrača, oko njega je crna kutija, predmet stoji na pivotu,
osvetljen sa dva point light-a (key + fill), kamera se prebaci na stage preko
`SetViewTarget`. Kamera eksplicitno gasi DOF, motion blur, bloom, film grain,
chromatic aberration, Lumen GI i refleksije, i resetuje TSR/TAA istoriju pri
ulasku i izlasku.

Šta se briše/menja kod tebe lokalno:

- [ ] **NE treba** više `M_InspectBackgroundBlur` materijal (slobodno obriši
  ako si ga napravio), ni `BackgroundBlurMaterialPath` u `DefaultGame.ini`
  (linija se sada ignoriše, ali očisti radi reda).
- [ ] `r.CustomDepth=3` u `DefaultEngine.ini` više nije potreban za inspekciju
  (ostavi ga samo ako ti treba za nešto drugo).
- [ ] Recompile + smoke: E na predmet → crna pozadina, osvetljen predmet,
  rotacija mišem radi, Esc/E vraća igru i kameru na pawna.
- [ ] Ako je predmet pretaman/presvetao, štełuj u `DefaultGame.ini`:

  ```
  [/Script/Loop9.InspectionStageActor]
  ExposureBias=0.0              ; više = svetlije (u stopovima)
  KeyLightIntensityCandela=300.0
  FillLightIntensityCandela=75.0
  ```

#### Popravka 17.07. — ljubičasti blokovi + mekoća

Verovatni uzroci ljubičastih blokova bili su temporalna istorija nakon
teleporta kamere i/ili Lumen surface cache izvrnute kutije. Mekoću su mogli
praviti motion blur, DOF iz globalnog post-process volumena i TAA/TSR.

Šta je promenjeno u kodu (samo recompile, ništa u editoru):
- zidovi sobe su sada 6 običnih ravni okrenutih ka unutra (bez negativnog
  skejla), potpuno isključene iz osvetljenja;
- na stage kameri su ugašeni **Lumen GI i refleksije** (crnoj sobi ne trebaju,
  pa uklanjamo mogući izvor blokova);
- ugašeni **DOF, motion blur, bloom, film grain i chromatic aberration**,
  dodat blagi tonemapper **sharpen (0.4)**;
- camera cut resetuje TSR/TAA istoriju pri ulasku i izlasku;
- zidovi koriste hard-referenced `BasicShapeMaterial`, pa više ne zavise od
  necookovanog `EngineDebugMaterials` asseta.
- [ ] Smoke: rotiraj predmet 10-15 s po svim osama — nema ljubičastih
  blokova, ivice ostaju oštre i dok se predmet okreće.
- [ ] Ako je i dalje mekano proveri da li source materijal koristi translucency,
  dither ili world-position efekte; tada dodeli neprozirnu varijantu u
  `InspectionMaterialOverrides`.

## 5. Steamworks — ČEKA APP ID

- [ ] App ID u `DefaultEngine.ini` + Web API Key → Render
- [ ] 28 achievements (+ Scale/Clock/Phantom ikone)
- [ ] Publish

## 6. Backend — GOTOVO za telemetry + production hardening

- [x] Telemetry E2E (HTTP 204)
- [x] Production hardening (fail-closed Steam auth, `/readyz`, body/JSON bounds,
  AI deadline/redaction, deploy PHPUnit gate) — backend PHPUnit green
- [ ] Pre release: Render Starter / keep-alive (cold start)
- [ ] Quota alarm

## 7. Store page (može paralelno bez App ID-a)

- [ ] Iseći capsule iz `Marketing/Steam/`
- [ ] Screenshotovi (min. 5)
- [ ] Trailer kasnije

## 8. QA (može paralelno)

- [ ] Shipping / packaged Steam build na čistoj mašini
- [ ] **Standalone** cold launch: first chat waits for Steam session; auth fail / offline refund
- [ ] Loop 9 clamp + success-only AI interaction / telemetry counts
- [ ] Offline chat + cold start chat (client timeout 65s)
- [ ] 10-min Unreal Insights + `stat unit` / `stat game` / `stat gpu` (desktop + Deck)
- [ ] Alt-Tab / Steam Overlay
- [ ] Deck QA
- [ ] Pakovani build bez tokena u `DefaultGame.ini`

## 9. Production/perf hardening — editor recompile (home machine)

Novi C++ (auth gate, endpoint utils, tick throttles, sound-mix pop/re-push). Posle
`Loop9Editor` Development rebuild:

- [ ] Standalone Game + Steam: first message authorize-then-dispatch
- [ ] Za lokalni non-Steam backend test eksplicitno postavi
  `[/Script/Loop9.Loop9BackendAuthSubsystem] bRequireSteamSession=false`;
  Shipping/prod mora ostati podrazumevano `true`.
- [ ] Pause clears interaction prompt immediately; prompt trace ~20 Hz in play
- [ ] Ambient volume still responds via sound-mix (no permanent `SC_Music` asset edit)
- [ ] Doors / lift wings idle without ticks; blink overlay only ticks while playing
