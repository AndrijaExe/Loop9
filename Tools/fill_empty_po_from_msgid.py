"""Copy non-empty msgstr onto empty PO entries that share the same msgid."""

from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1] / "Content" / "Localization" / "Game"
SKIP_MSGIDS = {
    "",
    "AO",
    "Roughness",
    "Metallic",
    "Sub UVAnimation",
    "EXPLOSION AREA",
    "Button Text",
    "Label",
    "100%",
    "Text Block",
    ".",
    "{0}%",
    "???",
}

ENTRY_RE = re.compile(
    r"(?P<head>(?:#.*\n)*)"
    r"(?P<ctx>msgctxt \".*?\"\n)?"
    r"msgid (?P<id>\"\"|(?:\"(?:\\.|[^\"\\])*\"(?:\n\"(?:\\.|[^\"\\])*\")*))\n"
    r"msgstr (?P<str>\"\"|(?:\"(?:\\.|[^\"\\])*\"(?:\n\"(?:\\.|[^\"\\])*\")*))",
    re.M,
)


def unescape_po(literal: str) -> str:
    parts = re.findall(r"\"((?:\\.|[^\"\\])*)\"", literal)
    raw = "".join(parts)
    return (
        raw.replace("\\\\", "\0")
        .replace("\\n", "\n")
        .replace("\\t", "\t")
        .replace("\\r", "\r")
        .replace('\\"', '"')
        .replace("\0", "\\")
    )


def escape_po(text: str) -> str:
    escaped = (
        text.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "\\r")
        .replace("\t", "\\t")
    )
    return f'"{escaped}"'


def fill_file(path: Path) -> int:
    text = path.read_text(encoding="utf-8")
    matches = list(ENTRY_RE.finditer(text))
    by_msgid: dict[str, tuple[str, str]] = {}
    parsed: list[tuple[re.Match[str], str, str]] = []
    for match in matches:
        msgid = unescape_po(match.group("id"))
        msgstr = unescape_po(match.group("str"))
        parsed.append((match, msgid, msgstr))
        ctx = match.group("ctx") or ""
        if msgstr.strip() and msgid not in SKIP_MSGIDS:
            previous = by_msgid.get(msgid)
            if previous is None or ("Loop9" in ctx and "Loop9" not in (previous[0] or "")):
                by_msgid[msgid] = (ctx, msgstr)

    filled = 0
    new = text
    for match, msgid, msgstr in reversed(parsed):
        if msgstr.strip() or msgid in SKIP_MSGIDS:
            continue
        hit = by_msgid.get(msgid)
        if not hit:
            continue
        new = new[: match.start("str")] + escape_po(hit[1]) + new[match.end("str") :]
        filled += 1

    if new != text:
        path.write_text(new, encoding="utf-8", newline="\n")
    return filled


def main() -> None:
    for culture in ("sr", "de", "fr", "ru"):
        path = ROOT / culture / "Game.po"
        filled = fill_file(path)
        print(f"{culture}: filled {filled} empty msgstr from matching msgid")


if __name__ == "__main__":
    main()
