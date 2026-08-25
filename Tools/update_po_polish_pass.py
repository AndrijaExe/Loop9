"""Apply the Help rewording and the new SKIP label to every Game.po.

Editing the PO files by hand across five languages is how msgid drift happens:
one stray character and GatherText treats the entry as new and drops the
translation. This does exact-string replacement and fails loudly if a source
string is not found, so a mismatch is a crash rather than a silent regression.
"""

from __future__ import annotations

import sys
from pathlib import Path

PO_ROOT = Path(__file__).resolve().parents[1] / "Content" / "Localization" / "Game"
CULTURES = ("en", "sr", "de", "fr", "ru")

OLD_ANOMALY_HEADING = "WHAT COUNTS AS AN ANOMALY"
NEW_ANOMALY_HEADING = "WHAT DOES NOT COUNT AS AN ANOMALY"

OLD_ANOMALY_BODY = (
    "An anomaly is any single thing the office would not do on its own. An object that is "
    "missing. An object standing somewhere it never stood. A poster or a screen whose text has "
    "changed. A light that will not stop flickering, and that you can hear from down the "
    "corridor. A door that is locked when it never was. A sound with nothing making it. "
    "Something that is the wrong size. A message on your phone that nobody sent. And, rarely, "
    "something on the floor with you."
)
NEW_ANOMALY_BODY = (
    "An anomaly is something the office would not do on its own, and the floor is full of "
    "things that look wrong without being wrong. The loop number on your screen counts up every "
    "loop because that is what a counter does; it is never the anomaly. The office is always "
    "this dark, and your own flashlight is not evidence. Dragojlo calling you is normal. So is "
    "one elevator being lit and the other dark. If you cannot tell whether something belongs, "
    "the first loop already answered it: if it was there on loop one, it belongs."
)

OLD_ELEVATOR_BODY = (
    "This is the whole game, so read it twice. If you found an anomaly, take the LIT elevator. "
    "If the floor is clean, take the DARK elevator. Get it right and you move up one loop. Get "
    "it wrong and you go back to the beginning. Reaching loop 9 ends your shift, and how you "
    "got there decides which ending you get."
)
NEW_ELEVATOR_BODY = (
    "This is the whole game, so read it twice. If you found an anomaly, take the LIT elevator. "
    "If the floor is clean, take the DARK elevator. Get it right and you move up one loop. Get "
    "it wrong and you go back to the beginning. Reach loop 9 to end your shift and find out "
    "which ending you get."
)

# msgstr replacements per culture. English msgstr mirrors the msgid.
TRANSLATIONS = {
    "en": {
        "heading": NEW_ANOMALY_HEADING,
        "body": NEW_ANOMALY_BODY,
        "elevator": NEW_ELEVATOR_BODY,
        "skip": "SKIP",
    },
    "sr": {
        "heading": "ŠTA SE NE RAČUNA KAO ANOMALIJA",
        "body": (
            "Anomalija je nešto što kancelarija ne bi uradila sama, a sprat je pun stvari koje "
            "izgledaju pogrešno a nisu. Broj petlje na ekranu raste svake petlje jer to brojač "
            "i radi; on nikada nije anomalija. Kancelarija je uvek ovoliko mračna, a tvoja "
            "sopstvena baterijska lampa nije dokaz. Normalno je da te Dragojlo zove. Normalno "
            "je i da je jedan lift osvetljen a drugi mračan. Ako ne umeš da odlučiš da li nešto "
            "pripada tu, prva petlja je već odgovorila: ako je bilo tu u prvoj petlji, pripada."
        ),
        "elevator": (
            "Ovo je cela igra, pa pročitaj dvaput. Ako si našao anomaliju, uđi u OSVETLJENI "
            "lift. Ako je sprat čist, uđi u MRAČNI lift. Pogodiš li, ideš petlju dalje. "
            "Pogrešiš li, vraćaš se na početak. Stigni do devete petlje da završiš smenu i "
            "vidiš koji kraj dobijaš."
        ),
        "skip": "PRESKOČI",
    },
    "de": {
        "heading": "WAS NICHT ALS ANOMALIE ZÄHLT",
        "body": (
            "Eine Anomalie ist etwas, das das Büro nicht von sich aus tun würde, und die Etage "
            "ist voll von Dingen, die falsch aussehen, ohne falsch zu sein. Die Schleifennummer "
            "auf dem Bildschirm zählt jede Schleife hoch, weil ein Zähler genau das tut; sie "
            "ist niemals die Anomalie. Das Büro ist immer so dunkel, und deine eigene "
            "Taschenlampe ist kein Beweis. Dass Dragojlo dich anruft, ist normal. Dass ein "
            "Aufzug erleuchtet und der andere dunkel ist, ebenfalls. Wenn du nicht entscheiden "
            "kannst, ob etwas hierher gehört, hat die erste Schleife es schon beantwortet: war "
            "es in Schleife eins da, gehört es hierher."
        ),
        "elevator": (
            "Das ist das ganze Spiel, also lies es zweimal. Hast du eine Anomalie gefunden, "
            "nimm den ERLEUCHTETEN Aufzug. Ist die Etage sauber, nimm den DUNKLEN Aufzug. "
            "Stimmt es, rückst du eine Schleife vor. Stimmt es nicht, fängst du von vorn an. "
            "Erreiche Schleife 9, um deine Schicht zu beenden und herauszufinden, welches Ende "
            "du bekommst."
        ),
        "skip": "ÜBERSPRINGEN",
    },
    "fr": {
        "heading": "CE QUI NE COMPTE PAS COMME ANOMALIE",
        "body": (
            "Une anomalie est quelque chose que le bureau ne ferait pas de lui-même, et l'étage "
            "est plein de choses qui semblent anormales sans l'être. Le numéro de boucle "
            "affiché à l'écran augmente à chaque boucle parce que c'est le rôle d'un compteur ; "
            "il n'est jamais l'anomalie. Le bureau est toujours aussi sombre, et ta propre "
            "lampe torche n'est pas une preuve. Que Dragojlo t'appelle est normal. Qu'un "
            "ascenseur soit éclairé et l'autre sombre l'est aussi. Si tu n'arrives pas à dire "
            "si quelque chose est à sa place, la première boucle a déjà répondu : si c'était là "
            "à la boucle un, c'est à sa place."
        ),
        "elevator": (
            "C'est tout le jeu, alors lis-le deux fois. Si tu as trouvé une anomalie, prends "
            "l'ascenseur ÉCLAIRÉ. Si l'étage est propre, prends l'ascenseur SOMBRE. Si tu as "
            "raison, tu montes d'une boucle. Si tu te trompes, tu repars du début. Atteins la "
            "boucle 9 pour terminer ton service et découvrir quelle fin tu obtiens."
        ),
        "skip": "PASSER",
    },
    "ru": {
        "heading": "ЧТО НЕ СЧИТАЕТСЯ АНОМАЛИЕЙ",
        "body": (
            "Аномалия — это то, чего офис не сделал бы сам, а на этаже полно вещей, которые "
            "выглядят неправильно, но неправильными не являются. Номер петли на экране растёт "
            "с каждой петлёй, потому что счётчик именно это и делает; он никогда не бывает "
            "аномалией. Офис всегда настолько тёмный, и твой собственный фонарь — не улика. То, "
            "что Драгойло звонит тебе, — нормально. Как и то, что один лифт освещён, а другой "
            "тёмный. Если не можешь понять, на месте ли что-то, первая петля уже ответила: если "
            "оно было там в первой петле, значит, оно на месте."
        ),
        "elevator": (
            "Это вся игра, так что прочти дважды. Если нашёл аномалию — садись в ОСВЕЩЁННЫЙ "
            "лифт. Если этаж чист — в ТЁМНЫЙ. Угадал — поднимаешься на петлю выше. Ошибся — "
            "возвращаешься к началу. Дойди до девятой петли, чтобы закончить смену и узнать, "
            "какую концовку ты получишь."
        ),
        "skip": "ПРОПУСТИТЬ",
    },
}

SKIP_ENTRY_TEMPLATE = """#. Key:\tSkipTypingLabel
#. SourceLocation:\tSource/Loop9/UI/EndingWidget.h(79)
#: Source/Loop9/UI/EndingWidget.h(79)
msgctxt "Loop9Endings,SkipTypingLabel"
msgid "SKIP"
msgstr "{skip}"

"""

CONTINUE_ANCHOR = '#. Key:\tContinueButtonLabel'


def replace_once(text: str, old: str, new: str, label: str, culture: str) -> str:
    if old not in text:
        raise SystemExit(f"{culture}: could not find {label}:\n  {old[:90]}...")
    return text.replace(old, new, 1)


def process(culture: str) -> None:
    path = PO_ROOT / culture / "Game.po"
    text = path.read_text(encoding="utf-8-sig")
    strings = TRANSLATIONS[culture]

    # msgid lines are shared by every culture; msgstr lines are per culture.
    for old, new, label in (
        (f'msgid "{OLD_ANOMALY_HEADING}"', f'msgid "{NEW_ANOMALY_HEADING}"', "anomaly heading msgid"),
        (f'msgid "{OLD_ANOMALY_BODY}"', f'msgid "{NEW_ANOMALY_BODY}"', "anomaly body msgid"),
        (f'msgid "{OLD_ELEVATOR_BODY}"', f'msgid "{NEW_ELEVATOR_BODY}"', "elevator body msgid"),
    ):
        text = replace_once(text, old, new, label, culture)

    # The old msgstr values are the previous translations, which differ per file,
    # so target them by the msgid that now precedes them.
    for msgid, msgstr, label in (
        (NEW_ANOMALY_HEADING, strings["heading"], "anomaly heading msgstr"),
        (NEW_ANOMALY_BODY, strings["body"], "anomaly body msgstr"),
        (NEW_ELEVATOR_BODY, strings["elevator"], "elevator body msgstr"),
    ):
        start = text.find(f'msgid "{msgid}"')
        if start == -1:
            raise SystemExit(f"{culture}: {label} anchor missing")
        line_start = text.index("\n", start) + 1
        line_end = text.index("\n", line_start)
        if not text[line_start:line_end].startswith("msgstr "):
            raise SystemExit(f"{culture}: {label} is not followed by msgstr")
        text = text[:line_start] + f'msgstr "{msgstr}"' + text[line_end:]

    if "Loop9Endings,SkipTypingLabel" not in text:
        anchor = text.find(CONTINUE_ANCHOR)
        if anchor == -1:
            raise SystemExit(f"{culture}: ContinueButtonLabel anchor missing")
        text = text[:anchor] + SKIP_ENTRY_TEMPLATE.format(skip=strings["skip"]) + text[anchor:]

    path.write_text(text, encoding="utf-8-sig")
    print(f"{culture}: updated")


def main() -> None:
    for culture in CULTURES:
        process(culture)
    return 0


if __name__ == "__main__":
    sys.exit(main())
