# SMENA — žig studija

Wordmark studija, izabran 28.08.2026. „Smena“ = radna smena; igra je noćna smena
koja se ne završava, pa ime nosi temu, a ne dekoraciju.

**Wordmark studija i ikonica igre nisu ista stvar.** Ovo ovde ide na boot splash,
u kredite i na store/social materijale. `Build/Windows/Application.ico` je ikonica
**Loop 9**, ne studija — šest slova u kvadratu 32×32 px se ne može pročitati, a u
taskbaru treba da stoji igra. Za nju je izvor „Shortcut ICO v2“.

## Fajlovi

| Fajl | Kad ga koristiš |
|---|---|
| `smena-wordmark.svg` | Primarni. Ink `#F2F0EC` + fosforni kursor `#7CE0A0`. |
| `smena-wordmark-white.svg` | Jedna boja na tamnom. Krediti, footer. |
| `smena-wordmark-black.svg` | Jedna boja na svetlom. Štampa, svetli UI. |
| `smena-wordmark-currentcolor.svg` | Za inline u HTML — nasleđuje `color`. Ne radi kroz `<img>`. |
| `png/smena-wordmark-*.png` | Transparentni PNG, širine 256–2048. Ista dva varijanta boje. |
| `wordmark-proof.png` | Kontrolni list izabrane varijante: mala veličina, jedna boja, boot okvir. |
| `wordmark-explorations.png` | Šest smerova od kojih je izabran 02. Čuva se kao zapis odluke. |

## Slova su putanje, ne tekst

Glifovi su konvertovani u SVG putanje, pa žig **ne zavisi od fonta** na mašini
koja ga renderuje. Zato ga ne možeš editovati kao tekst — to je namerno, tako se
ne raspadne nigde i kerning ostaje zaključan.

Izvor: **Share Tech Mono**, Carrois Type Design / Ralph du Carrois, pod SIL Open
Font License 1.1 (`ShareTechMono-OFL.txt`). OFL dozvoljava komercijalnu upotrebu,
uključujući logotipe, i ne traži atribuciju u igri. Kopija licence stoji tu jer je
OFL zahteva uz distribuciju samog fonta.

## Regenerisanje

`build_wordmark.py` traži `ShareTechMono-Regular.ttf` pored sebe (nije u repou,
jer gotovi žig ne zavisi od njega) i `fonttools`:

```bash
curl -sLO https://github.com/google/fonts/raw/main/ofl/sharetechmono/ShareTechMono-Regular.ttf
python3 build_wordmark.py     # traži fonttools
```

Tracking je `0.40em` (`TRACKING_EM`), to je jedini broj koji menja karakter žiga.

## Još nije rešeno

Pre nego što ime ode na store stranicu, mora provera kolizije: Steam publisher
pretraga, osnovna trademark pretraga (EUIPO + USPTO), domen i social handle. Ime
pod kojim nastupaš i pravno lice su dve različite stvari — ne treba registrovana
firma da bi se zvao SMENA, ali treba da nema sudara.

Kad prođe, `Marketing/Steam/STORE_PAGE.md` i Steamworks Developer / Publisher
polje idu sa „Andrija Stanišić (ili ime studija…)“ na finalno ime.
