# -*- coding: utf-8 -*-
from pathlib import Path

ROOT = Path(r"D:\UE Course\Loop 9 AI\Loop9 5.8\Content\Localization\Game")

TRANSLATIONS = {
    "sr": {
        "SETTINGS": "PODEŠAVANJA",
        "Window Mode": "Režim prozora",
        "Resolution": "Rezolucija",
        "Graphics Quality": "Kvalitet grafike",
        "Resolution Scale": "Skala rezolucije",
        "FPS Limit": "Limit FPS",
        "VSync": "VSync",
        "Brightness (Gamma)": "Osvetljenje (gama)",
        "Language": "Jezik",
        "Master Volume": "Glavna jačina",
        "Ambient Volume": "Jačina ambijenta",
        "Mouse Sensitivity": "Osetljivost miša",
        "Invert Y Axis": "Invertuj Y osu",
        "APPLY": "PRIMENI",
        "BACK": "NAZAD",
        "PLAY": "IGRAJ",
        "QUIT": "IZAĐI",
        "PAUSED": "PAUZIRANO",
        "RESUME": "NASTAVI",
        "MAIN MENU": "GLAVNI MENI",
    },
    "de": {
        "SETTINGS": "EINSTELLUNGEN",
        "Window Mode": "Fenstermodus",
        "Resolution": "Auflösung",
        "Graphics Quality": "Grafikqualität",
        "Resolution Scale": "Auflösungsskala",
        "FPS Limit": "FPS-Limit",
        "VSync": "VSync",
        "Brightness (Gamma)": "Helligkeit (Gamma)",
        "Language": "Sprache",
        "Master Volume": "Hauptlautstärke",
        "Ambient Volume": "Umgebungslautstärke",
        "Mouse Sensitivity": "Mausempfindlichkeit",
        "Invert Y Axis": "Y-Achse umkehren",
        "APPLY": "ANWENDEN",
        "BACK": "ZURÜCK",
        "PLAY": "SPIELEN",
        "QUIT": "BEENDEN",
        "PAUSED": "PAUSIERT",
        "RESUME": "FORTSETZEN",
        "MAIN MENU": "HAUPTMENÜ",
        "Windowed": "Fenster",
        "Fullscreen": "Vollbild",
        "Borderless": "Rahmenlos",
        "Low": "Niedrig",
        "Medium": "Mittel",
        "High": "Hoch",
        "Epic": "Episch",
        "Uncapped": "Unbegrenzt",
    },
    "fr": {
        "SETTINGS": "PARAMÈTRES",
        "Window Mode": "Mode fenetre",
        "Resolution": "Resolution",
        "Graphics Quality": "Qualite graphique",
        "Resolution Scale": "Echelle de resolution",
        "FPS Limit": "Limite FPS",
        "VSync": "VSync",
        "Brightness (Gamma)": "Luminosite (gamma)",
        "Language": "Langue",
        "Master Volume": "Volume principal",
        "Ambient Volume": "Volume ambiant",
        "Mouse Sensitivity": "Sensibilite de la souris",
        "Invert Y Axis": "Inverser l'axe Y",
        "APPLY": "APPLIQUER",
        "BACK": "RETOUR",
        "PLAY": "JOUER",
        "QUIT": "QUITTER",
        "PAUSED": "PAUSE",
        "RESUME": "REPRENDRE",
        "MAIN MENU": "MENU PRINCIPAL",
        "Windowed": "Fenetre",
        "Fullscreen": "Plein ecran",
        "Borderless": "Sans bordure",
        "Low": "Faible",
        "Medium": "Moyen",
        "High": "Eleve",
        "Epic": "Epique",
        "Uncapped": "Illimite",
    },
    "ru": {
        "SETTINGS": "НАСТРОЙКИ",
        "Window Mode": "Режим окна",
        "Resolution": "Разрешение",
        "Graphics Quality": "Качество графики",
        "Resolution Scale": "Масштаб разрешения",
        "FPS Limit": "Лимит FPS",
        "VSync": "VSync",
        "Brightness (Gamma)": "Яркость (гамма)",
        "Language": "Язык",
        "Master Volume": "Общая громкость",
        "Ambient Volume": "Громкость окружения",
        "Mouse Sensitivity": "Чувствительность мыши",
        "Invert Y Axis": "Инверсия оси Y",
        "APPLY": "ПРИМЕНИТЬ",
        "BACK": "НАЗАД",
        "PLAY": "ИГРАТЬ",
        "QUIT": "ВЫХОД",
        "PAUSED": "ПАУЗА",
        "RESUME": "ПРОДОЛЖИТЬ",
        "MAIN MENU": "ГЛАВНОЕ МЕНЮ",
        "Windowed": "В окне",
        "Fullscreen": "Полный экран",
        "Borderless": "Без рамок",
        "Low": "Низкое",
        "Medium": "Среднее",
        "High": "Высокое",
        "Epic": "Эпическое",
        "Uncapped": "Без ограничений",
    },
}

KEYS = [
    ("Loop9Settings", "Title", "SETTINGS"),
    ("Loop9Settings", "WindowMode", "Window Mode"),
    ("Loop9Settings", "Resolution", "Resolution"),
    ("Loop9Settings", "GraphicsQuality", "Graphics Quality"),
    ("Loop9Settings", "ResolutionScale", "Resolution Scale"),
    ("Loop9Settings", "FPSLimit", "FPS Limit"),
    ("Loop9Settings", "VSync", "VSync"),
    ("Loop9Settings", "Brightness", "Brightness (Gamma)"),
    ("Loop9Settings", "Language", "Language"),
    ("Loop9Settings", "MasterVolume", "Master Volume"),
    ("Loop9Settings", "AmbientVolume", "Ambient Volume"),
    ("Loop9Settings", "MouseSensitivity", "Mouse Sensitivity"),
    ("Loop9Settings", "InvertY", "Invert Y Axis"),
    ("Loop9Settings", "Apply", "APPLY"),
    ("Loop9Settings", "Back", "BACK"),
    ("Loop9Settings", "WindowModeWindowed", "Windowed"),
    ("Loop9Settings", "WindowModeFullscreen", "Fullscreen"),
    ("Loop9Settings", "WindowModeBorderless", "Borderless"),
    ("Loop9Settings", "QualityLow", "Low"),
    ("Loop9Settings", "QualityMedium", "Medium"),
    ("Loop9Settings", "QualityHigh", "High"),
    ("Loop9Settings", "QualityEpic", "Epic"),
    ("Loop9Settings", "FPSUncapped", "Uncapped"),
    ("Loop9Menu", "Play", "PLAY"),
    ("Loop9Menu", "Settings", "SETTINGS"),
    ("Loop9Menu", "Quit", "QUIT"),
    ("Loop9Menu", "Paused", "PAUSED"),
    ("Loop9Menu", "Resume", "RESUME"),
    ("Loop9Menu", "MainMenu", "MAIN MENU"),
]


def esc(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def entry(ns: str, key: str, en: str, tr: str) -> str:
    return (
        f"\n#. Key:\t{key}\n"
        f"#. SourceLocation:\tSource/Loop9\n"
        f'msgctxt "{ns},{key}"\n'
        f'msgid "{esc(en)}"\n'
        f'msgstr "{esc(tr)}"\n'
    )


def main() -> None:
    # Append missing keys to Serbian PO
    sr_path = ROOT / "sr" / "Game.po"
    sr_text = sr_path.read_text(encoding="utf-8")
    extra = []
    for ns, key, en in KEYS:
        needle = f'msgctxt "{ns},{key}"'
        if needle not in sr_text:
            tr = TRANSLATIONS["sr"].get(en, en)
            extra.append(entry(ns, key, en, tr))
    if extra:
        with sr_path.open("a", encoding="utf-8") as f:
            f.write("".join(extra))
        print(f"Appended {len(extra)} entries to sr/Game.po")

    for lang in ("de", "fr", "ru"):
        lang_dir = ROOT / lang
        lang_dir.mkdir(parents=True, exist_ok=True)
        lines = [
            f"# Game {lang} translation.\n",
            'msgid ""\n',
            "msgstr \"\"\n",
            '"Project-Id-Version: Game\\n"\n',
            f'"Language: {lang}\\n"\n',
            '"MIME-Version: 1.0\\n"\n',
            '"Content-Type: text/plain; charset=UTF-8\\n"\n',
            '"Content-Transfer-Encoding: 8bit\\n"\n',
            "\n",
        ]
        for ns, key, en in KEYS:
            tr = TRANSLATIONS[lang].get(en, en)
            lines.append(entry(ns, key, en, tr))
        (lang_dir / "Game.po").write_text("".join(lines), encoding="utf-8")
        print(f"Wrote {lang}/Game.po")


if __name__ == "__main__":
    main()
