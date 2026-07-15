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
