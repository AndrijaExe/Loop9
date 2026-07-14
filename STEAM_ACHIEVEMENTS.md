# Loop 9 — Steam achievementi

Kompletna lista achievementa koje igra otključava, plus tačan postupak kako ih
uneseš u Steamworks kad budeš objavljivao igru. Kod u igri je već povezan —
tvoj posao je samo da svaki achievement iz tabele definišeš u Steamworks-u sa
**identičnim API Name**.

## Kako sistem radi (ukratko)

- `Loop9AchievementsSubsystem` prati gameplay događaje (odluke kod lifta,
  poruke Dragojlu, loopove, endinge) i šalje unlock Steamu preko Online
  Subsystem-a.
- Bez Steama (dev build, drugi launcher) sve je no-op — ništa ne puca.
- Napredak koji se skuplja kroz više prolaza (viđeni endinzi, uočene anomalije)
  čuva se u `Saved/Config/.../Game.ini`, pa meta-achievementi rade i ako igrač
  restartuje igru.
- Dodatne achievemente možeš okidati i iz Blueprinta:
  `Get Game Instance Subsystem (Loop9AchievementsSubsystem)` →
  `UnlockAchievement("ACH_NEKO_IME")`.

---

## Lista achievementa (25)

Kolona "Hidden" = označi kao skriven u Steamworks-u (spoiler). Predlozi imena i
opisa su na engleskom (Steam prikazuje lokalizaciju kasnije ako je dodaš).

### Endinzi (svi hidden — spoileri)

| # | API Name | Display Name | Opis (prikazuje se igraču) | Hidden |
|---|----------|--------------|---------------------------|--------|
| 1 | `ACH_ENDING_ESCAPE_TOGETHER` | Escape Together | Leave the building with Dragojlo. | DA |
| 2 | `ACH_ENDING_OBEDIENT_FOOL` | Obedient Fool | Trust him. Completely. To the end. | DA |
| 3 | `ACH_ENDING_COLD_BETRAYAL` | Cold Betrayal | Some colleagues don't forgive. | DA |
| 4 | `ACH_ENDING_PARANOID_SURVIVOR` | Paranoid Survivor | Trust no one. Not even the phone. | DA |
| 5 | `ACH_ENDING_MERGED_MEMORY` | Merged Memory | Where does he end and you begin? | DA |
| 6 | `ACH_ENDING_THE_REPLACEMENT` | The Replacement | Someone has to answer the phone. | DA |
| 7 | `ACH_ALL_ENDINGS` | Every Shift Ends | See all six endings. | NE |

**Uslov u kodu:** 1–6 se otključavaju automatski kad se prikaže odgovarajući
ending; 7 kad su svi iz 1–6 viđeni (kroz bilo koji broj prolaza).

### Progresija

| # | API Name | Display Name | Opis | Hidden |
|---|----------|--------------|------|--------|
| 8 | `ACH_FINISH_RUN` | End of Shift | Finish the game once. | NE |
| 9 | `ACH_FIRST_CALL` | Hello? Who Is This? | Talk to Dragojlo for the first time. | NE |
| 10 | `ACH_FIRST_RESET` | Back to One | Take the wrong elevator. | NE |
| 11 | `ACH_REACH_LOOP_5` | Halfway There | Reach loop 5. | NE |

**Uslov u kodu:** 8 na bilo koji ending; 9 na prvu poslatu poruku AI-ju;
10 na prvu pogrešnu odluku (povratak na sprat 1); 11 kad loop brojač
stigne do 5.

### Veština

| # | API Name | Display Name | Opis | Hidden |
|---|----------|--------------|------|--------|
| 12 | `ACH_PERFECT_RUN` | Spotless Record | Finish the game without a single wrong call. | NE |
| 13 | `ACH_STREAK_7` | Sharp Eye | Make 7 correct elevator calls in a row. | NE |
| 14 | `ACH_SILENT_RUN` | Who Needs Dragojlo | Finish the game without ever answering the phone chat. | NE |
| 15 | `ACH_HOTLINE` | Hotline | Talk to Dragojlo 15 times in a single run. | NE |
| 16 | `ACH_GROUNDHOG` | Groundhog Shift | Get sent back to floor 1 ten times in one run. | NE |
| 17 | `ACH_DEJA_VU` | Déjà Vu | Correctly call out an anomaly that repeated from the previous loop. | DA |

**Uslov u kodu:** 12 — ending sa 0 resetova; 13 — brojač uzastopnih tačnih
odluka (pogrešna resetuje niz); 14 — ending sa 0 AI poruka; 15 — 15. poruka u
jednom prolazu; 16 — 10. reset u jednom prolazu; 17 — tačna "lit elevator"
odluka dok je aktivna repeat anomalija.

### Lov na anomalije

| # | API Name | Display Name | Opis | Hidden |
|---|----------|--------------|------|--------|
| 18 | `ACH_SPOT_HIDE` | Something's Missing | Correctly call out a hidden-object anomaly. | NE |
| 19 | `ACH_SPOT_MOVE` | That Wasn't There | Correctly call out a moved-object anomaly. | NE |
| 20 | `ACH_SPOT_LIGHT` | Flicker | Correctly call out a light anomaly. | NE |
| 21 | `ACH_SPOT_AUDIO` | Do You Hear That | Correctly call out an audio anomaly. | NE |
| 22 | `ACH_SPOT_TEXT` | Read Between the Lines | Correctly call out a text anomaly. | NE |
| 23 | `ACH_SPOT_DOORLOCK` | Locked Out | Correctly call out a door anomaly. | NE |
| 24 | `ACH_SPOT_PURSUER` | Don't Look Back | Survive a pursuer and call it out. | NE |
| 25 | `ACH_SPOT_ALL` | Anomaly Almanac | Correctly call out every type of anomaly. | NE |

**Uslov u kodu:** 18–24 — tačna odluka (lit elevator) dok je aktivna anomalija
tog tipa; 25 — svih 7 tipova uočeno (kumulativno kroz prolaze, persistovano).

---

## Postupak u Steamworks-u (korak po korak)

Preduslov: imaš svoj App ID (Steam Direct, $100). Achievementi **ne rade na
Spacewar/480** — testiranje ide tek sa tvojim App ID-om.

### A. Priprema ikonica (pre unosa)

Za svaki achievement trebaju **dve ikonice, 256×256 px, JPG ili 24-bit PNG
(bez providnosti)**:

- **Achieved** (u boji) — prikazuje se kad se otključa
- **Locked** (siva varijanta) — prikazuje se dok je zaključan

Praktičan pristup: napravi jednu baznu ikonicu po grupi (ending / progresija /
veština / anomalije) u varijacijama, ili 25 jedinstvenih ako imaš vremena.
Imenuj fajlove po API imenu (`ACH_SPOT_HIDE_on.png`, `ACH_SPOT_HIDE_off.png`)
da ne pomešaš pri uploadu.

### B. Unos achievementa (ponavljaš za svaki red iz tabele)

1. Otvori [partner.steamgames.com](https://partner.steamgames.com) i uloguj se.
2. **Apps & Packages → All Applications →** izaberi Loop 9.
3. U meniju aplikacije: **Technical Tools → Edit Steamworks Settings**.
4. Tab **Stats & Achievements → Achievements**.
5. Klikni **"New Achievement"** i popuni:
   - **API Name** — prekopiraj tačno iz tabele (npr. `ACH_SPOT_HIDE`).
     VELIKA SLOVA, bez razmaka. Ovo je jedino polje koje kod vidi — greška
     ovde znači da se achievement nikad neće otključati.
   - **Display Name** — iz tabele (ili svoja varijanta).
   - **Description** — iz tabele.
   - **Hidden** — postavi na "Yes" za redove označene sa DA (endinzi i
     Déjà Vu). Skriveni achievementi prikazuju "???" dok se ne otključaju.
   - **Achieved Icon** / **Unachieved Icon** — upload dve ikonice.
6. **Save** posle svakog achievementa.
7. Kad uneseš svih 25, idi na **Publish** tab (u Steamworks Settings):
   - **Prepare for Publishing → Publish to Steam** (traži confirm kod).
   - Promene u Stats & Achievements NE VAŽE dok ne publish-uješ — ovo je
     najčešća greška ("uneo sam ali ne radi").

### C. Testiranje (pre release-a)

1. Uveri se da je build igre koji testiraš pokrenut **kroz Steam klijent**
   (dodaj exe kao non-Steam game NE radi — mora kroz tvoj App ID: ili
   `steam_appid.txt` sa tvojim App ID-om pored exe-a u dev buildu, ili upload
   builda na Steam i instalacija kroz klijent).
2. U igri izazovi uslov (npr. pošalji prvu poruku Dragojlu) i proveri:
   - Steam overlay notifikaciju u donjem desnom uglu,
   - log igre: `Achievements: unlock ACH_FIRST_CALL -> OK`.
3. Za ponovno testiranje resetuj svoje achievemente:
   **Steamworks → tvoja app → Stats & Achievements → "Reset achievements for
   your account"**, ili koristi `steam_testing` reset kroz Steam konzolu:
   `reset_all_stats <AppID>` (Steam klijent pokrenut sa `-console`).
4. Za meta-achievemente (svi endinzi / sve anomalije) obriši i lokalni
   napredak: sekcija `[/Script/Loop9.Loop9AchievementsSubsystem]` u
   `Saved/Config/<platforma>/Game.ini`.

### D. Checklist pre launcha

- [ ] Svih 25 API imena uneto slovo-za-slovo kao u tabeli
- [ ] Hidden flag postavljen za 7 spoiler achievementa (6 endinga + Déjà Vu)
- [ ] Sve ikonice (25 × 2) uploadovane
- [ ] Steamworks promene **publish-ovane**
- [ ] Svaka grupa testirana kroz Steam klijent (bar: jedan ending, first call,
      first reset, jedan spot, streak)
- [ ] `SteamDevAppId` u `Config/DefaultEngine.ini` zamenjen pravim App ID-om

---

## Dodavanje novih achievementa kasnije

1. Definiši API Name u Steamworks-u (koraci B) i publish-uj.
2. U igri: ili pozovi `UnlockAchievement("ACH_NOVO")` iz Blueprinta na
   odgovarajućem mestu, ili dodaj pravilo u `Loop9AchievementsSubsystem`
   (za uslove koji se prate kroz vreme).
3. Novi achievementi se mogu dodavati i posle release-a — Steam ih prikaže
   svim igračima nakon publish-a.
