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

## 4b. NOVO (15.07.): Item inspection (Resident Evil stil)

Novi feature u C++: pokupiš predmet pogledom + E, igra se pauzira, pozadina se
zamuti (depth-of-field, providna), predmet lebdi ispred kamere i rotira se
mišem (ili desnim stikom). Izlaz: **Esc / E / desni klik / B na gamepadu**.

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
- [ ] Smoke: E na predmet → pauza + blur + rotacija mišem; Esc vraća igru;
  pause meni ne može da se otvori dok traje inspekcija; ruke se ne vide
  tokom inspekcije, posle se vrate.

## 5. Steamworks — ČEKA APP ID

- [ ] App ID u `DefaultEngine.ini` + Web API Key → Render
- [ ] 28 achievements (+ Scale/Clock/Phantom ikone)
- [ ] Publish

## 6. Backend — GOTOVO za telemetry

- [x] Telemetry E2E (HTTP 204)
- [ ] Pre release: Render Starter / keep-alive (cold start)
- [ ] Quota alarm; `AUTH_ALLOW_GAME_TOKEN` off kad Steam auth radi

## 7. Store page (može paralelno bez App ID-a)

- [ ] Iseći capsule iz `Marketing/Steam/`
- [ ] Screenshotovi (min. 5)
- [ ] Trailer kasnije

## 8. QA (može paralelno)

- [ ] Shipping build na čistoj mašini
- [ ] Offline chat + cold start chat
- [ ] Alt-Tab / Steam Overlay
- [ ] Deck QA
- [ ] Pakovani build bez tokena u `DefaultGame.ini`
