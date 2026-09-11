"""Regenerate the Text-anomaly base-colour textures from the Deko source art.

The anomaly variants under Content/MyStuff/Anomalies were hand-painted on top
of the Deko_MatrixDemo magazine/newspaper/book textures. This script rebuilds
them from the clean base textures so the words read as part of the print
rather than as a sticker: a centred newspaper headline, a swapped headline
column, a back-cover teaser, a CRT prompt on the manual's monitor, and the
book's own title block.

Inputs (PNG exports of the base textures, 4096x4096) and outputs go through
one folder. Export the bases once from the editor (see docs/ANOMALIES.md,
"Text anomaly textures") and reimport the results over the existing assets.

    py -3 Tools/make_text_anomaly_textures.py <folder>

Requires Pillow and numpy.
"""
from __future__ import annotations

import random
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

FONTS = Path(r"C:\Windows\Fonts")
BASES = {
    "news": "Deko_MatrixDemo__Apartment__Textures__T_Newspaper_WobblyMag_OpenMagTight_A01_C.png",
    "mags": "Deko_MatrixDemo__Apartment__Textures__T_Magazines_Only_A01_C.png",
    "books": "Deko_MatrixDemo__Apartment__Textures__T_Post_It_A5_Books_A01_C.png",
}

PAPER = (204, 198, 184)
INK = (28, 25, 22)


def font(name: str, size: int, variation: bytes | None = None) -> ImageFont.FreeTypeFont:
    f = ImageFont.truetype(str(FONTS / name), size)
    if variation:
        try:
            f.set_variation_by_name(variation)
        except Exception:
            pass
    return f


def text_image(text: str, fnt: ImageFont.FreeTypeFont, color, squeeze: float = 1.0, pad: int = 8) -> Image.Image:
    """Render text tightly onto a transparent image, optionally squeezed horizontally."""
    l, t, r, b = fnt.getbbox(text)
    w, h = r - l + pad * 2, b - t + pad * 2
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(img).text((pad - l, pad - t), text, font=fnt, fill=color + (255,))
    if squeeze != 1.0:
        img = img.resize((max(1, int(w * squeeze)), h), Image.LANCZOS)
    return img


def fit_text(text: str, fontname: str, box_w: int, box_h: int, color, squeeze: float = 1.0,
             variation: bytes | None = None, max_size: int = 900) -> Image.Image:
    """Largest rendering of text that fits box_w x box_h."""
    size = max_size
    while size > 8:
        img = text_image(text, font(fontname, size, variation), color, squeeze)
        if img.width <= box_w and img.height <= box_h:
            return img
        size = int(size * 0.92)
    return text_image(text, font(fontname, 8, variation), color, squeeze)


def paste_center(dst: Image.Image, src: Image.Image, box) -> None:
    x0, y0, x1, y1 = box
    x = x0 + (x1 - x0 - src.width) // 2
    y = y0 + (y1 - y0 - src.height) // 2
    dst.alpha_composite(src, (x, y))


def grain(img: Image.Image, box, amount: float = 0.06, seed: int = 1) -> None:
    """Multiply a box of the image by soft noise so flat fills keep a paper feel."""
    x0, y0, x1, y1 = box
    rng = np.random.default_rng(seed)
    region = np.asarray(img.crop(box).convert("RGB")).astype(np.float32)
    noise = rng.normal(1.0, amount, size=region.shape[:2])[..., None]
    region = np.clip(region * noise, 0, 255).astype(np.uint8)
    img.paste(Image.fromarray(region).convert("RGBA"), (x0, y0))


def blend_box(img: Image.Image, base: Image.Image, box, color, keep: float = 0.18) -> None:
    """Fill a box with a flat colour but keep a little of the base texture in it."""
    x0, y0, x1, y1 = box
    flat = Image.new("RGBA", (x1 - x0, y1 - y0), color + (255,))
    under = base.crop(box).convert("RGBA")
    img.paste(Image.blend(flat, under, keep), (x0, y0))


# ---------------------------------------------------------------- newspaper (I01)

NEWS_PAGE_X = (1947, 3838)        # printed page between its border rules
NEWS_TOP_BAND = (1990, 96, 3800, 276)
NEWS_HEADLINE_COL = (2150, 190, 2382, 1990)   # the "DOWNTOWN BOMBING" column, subhead untouched


def news_top_band(base: Image.Image, text: str) -> Image.Image:
    img = base.copy()
    blend_box(img, base, NEWS_TOP_BAND, PAPER, keep=0.06)
    grain(img, NEWS_TOP_BAND, 0.04)
    x0, y0, x1, y1 = NEWS_TOP_BAND
    label = fit_text(text, "arialbd.ttf", x1 - x0 - 160, int((y1 - y0) * 0.58), INK, squeeze=0.92)
    # centred on the page, not on the band
    cx = (NEWS_PAGE_X[0] + NEWS_PAGE_X[1]) // 2
    img.alpha_composite(label, (cx - label.width // 2, y0 + (y1 - y0 - label.height) // 2))
    d = ImageDraw.Draw(img)
    d.line([(x0 + 40, y1 - 6), (x1 - 40, y1 - 6)], fill=INK + (255,), width=4)
    return img


def news_headline_column(base: Image.Image, text: str, shift: int = 0) -> Image.Image:
    """The paper's own headline column carries the words, rotated like the page."""
    img = base.copy()
    x0, y0, x1, y1 = NEWS_HEADLINE_COL
    box = (x0, y0 + shift, x1, y1)
    blend_box(img, base, box, PAPER, keep=0.0)
    grain(img, box, 0.045, seed=2)
    label = fit_text(text, "bahnschrift.ttf", (y1 - y0) - 140, (x1 - x0) - 40, INK, variation=b"Bold SemiCondensed")
    label = label.rotate(90, expand=True)
    paste_center(img, label, box)
    return img


# ---------------------------------------------------------------- magazine back cover (Magazine)

MAG_COVER = (60, 3000, 1490, 4040)


def magazine_teaser(base: Image.Image, word: str, variant: int) -> tuple[Image.Image, Image.Image]:
    """A 'next issue' teaser set in the cover's own condensed type, rotated like the cover.

    Returns (colour, emissive). The emissive is the word only, dim, so the old
    red glow does not survive under the new letters.
    """
    img = base.copy()
    x0, y0, x1, y1 = MAG_COVER
    rng = random.Random(variant)
    white = (236, 232, 220)
    grey = (150, 146, 138)

    # Small line first, then the word, both rotated 90 degrees like the cover text.
    small = text_image("IN THIS ISSUE", font("bahnschrift.ttf", 46, b"SemiBold SemiCondensed"), grey)
    small = small.rotate(90, expand=True)
    word_h = int((y1 - y0) * (0.62 if variant == 1 else 0.5 if variant == 2 else 0.72))
    word_w = int((x1 - x0) * 0.42)
    big = fit_text(word, "bahnschrift.ttf", word_h, word_w, white, variation=b"Bold Condensed")
    big = big.rotate(90, expand=True)

    # Column position varies per variant so the three swaps do not look identical.
    col_x = x0 + int((x1 - x0) * (0.30 if variant == 1 else 0.55 if variant == 2 else 0.42))
    top_y = y0 + int((y1 - y0) * (0.18 if variant == 1 else 0.30 if variant == 2 else 0.12))
    img.alpha_composite(small, (col_x - small.width - 28, top_y))
    img.alpha_composite(big, (col_x, top_y))
    d = ImageDraw.Draw(img)
    d.line([(col_x - 14, top_y), (col_x - 14, top_y + big.height)], fill=white + (255,), width=6)

    # Ink on black gets a faint halftone so it prints rather than glows.
    grain(img, (col_x - 80, top_y - 20, min(x1, col_x + big.width + 20), min(y1, top_y + big.height + 20)), 0.10, seed=variant)

    emissive = Image.new("RGBA", base.size, (0, 0, 0, 255))
    dim = big.copy()
    alpha = dim.split()[3].point(lambda a: int(a * 0.28))
    dim.putalpha(alpha)
    emissive.alpha_composite(dim, (col_x, top_y))
    return img, emissive


# ---------------------------------------------------------------- manual monitor (D01)

MON_SCREEN = (3490, 3150, 3805, 3362)


def monitor_prompt(base: Image.Image, word: str, variant: int) -> Image.Image:
    img = base.copy()
    x0, y0, x1, y1 = MON_SCREEN
    screen = Image.new("RGBA", (x1 - x0, y1 - y0), (6, 12, 8, 255))
    d = ImageDraw.Draw(screen)
    green = (120, 255, 150)
    dark_green = (40, 110, 60)
    f = font("consola.ttf", 34)
    lines = [
        "CP/M 2.2  64K",
        "A>DIR",
        "NO FILE",
        "A>",
    ]
    for i, line in enumerate(lines):
        d.text((14, 12 + i * 40), line, font=f, fill=dark_green + (255,))
    # The word is the only bright thing on the screen; the cursor sits after it.
    big = fit_text(word, "consola.ttf", (x1 - x0) - 40, 70, green)
    screen.alpha_composite(big, (14 + 34 * 2, 12 + 3 * 40 - 12))
    cursor_x = 14 + 34 * 2 + big.width + 8
    if variant != 3:
        d.rectangle([cursor_x, 12 + 3 * 40 + 6, cursor_x + 20, 12 + 3 * 40 + 42], fill=green + (255,))
    # Scanlines and a soft phosphor bloom.
    glow = screen.filter(ImageFilter.GaussianBlur(6))
    screen = Image.alpha_composite(glow, screen)
    arr = np.asarray(screen).astype(np.float32)
    arr[::3, :, :3] *= 0.72
    screen = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8))
    img.paste(screen, (x0, y0))
    return img


# ---------------------------------------------------------------- book title block (F01)

BOOK_TITLE = (2105, 2283, 2316, 3055)
BOOK_SPINE_TAG = (1360, 3050, 1534, 3172)
BOOK_GREEN = (0, 132, 111)
BOOK_CREAM = (220, 215, 184)


def book_title(base: Image.Image, word: str, variant: int) -> Image.Image:
    img = base.copy()
    d = ImageDraw.Draw(img)
    # Title block: same green, the word in the book's serif, rotated like the cover.
    d.rectangle(BOOK_TITLE, fill=BOOK_GREEN + (255,))
    x0, y0, x1, y1 = BOOK_TITLE
    label = fit_text(word, "timesbd.ttf", (y1 - y0) - 60, (x1 - x0) - 16, BOOK_CREAM)
    label = label.rotate(90, expand=True)
    paste_center(img, label, BOOK_TITLE)
    grain(img, BOOK_TITLE, 0.03, seed=variant)
    # Spine tag: small version of the same.
    d.rectangle(BOOK_SPINE_TAG, fill=BOOK_GREEN + (255,))
    sx0, sy0, sx1, sy1 = BOOK_SPINE_TAG
    tag = fit_text(word, "timesbd.ttf", (sx1 - sx0) - 20, (sy1 - sy0) - 24, BOOK_CREAM)
    paste_center(img, tag, BOOK_SPINE_TAG)
    return img


# ---------------------------------------------------------------- driver

def main(argv: list[str]) -> int:
    if len(argv) != 1:
        print(__doc__)
        return 1
    folder = Path(argv[0])
    bases = {}
    for key, name in BASES.items():
        path = folder / name
        if not path.is_file():
            print(f"missing base texture: {path}")
            return 1
        bases[key] = Image.open(path).convert("RGBA")

    out = folder / "generated"
    out.mkdir(exist_ok=True)

    def save(img: Image.Image, asset: str) -> None:
        img.convert("RGBA").save(out / f"{asset}.png", "PNG", optimize=False)
        print(asset)

    news = bases["news"]
    for word, asset in (("YOU ARE NEXT", "T_I01_YouAreNext"), ("HE IS IN THE OFFICE", "T_I01_HeIsInTheOffice")):
        save(news_top_band(news, word), f"{asset}_C")
        save(news_headline_column(news, word), f"{asset}_C2")
        save(news_headline_column(news, word, shift=60), f"{asset}_C3")

    for word, asset, variants in (("DIE", "T_MagazineAnomaly_Die", (1, 2, 3)),
                                  ("HELP", "T_MagazineAnomaly_Help", (1, 3)),
                                  ("RUN", "T_MagazineAnomaly_Run", (1, 3))):
        for v in variants:
            colour, emissive = magazine_teaser(news, word, v)
            suffix = "" if v == 1 else str(v)
            save(colour, f"{asset}_C{suffix}")
            # Only the emissives that exist as assets (E, and E2 for Die); the
            # materials never read an E3.
            if v == 1 or (v == 2 and asset.endswith("_Die")):
                save(emissive, f"{asset}_E{suffix}")

    mags = bases["mags"]
    for word, asset in (("DIE", "T_D01_Die"), ("HELP", "T_D01_Help"), ("RUN", "T_D01_Run")):
        save(monitor_prompt(mags, word, 1), f"{asset}_C")
        save(monitor_prompt(mags, word, 3), f"{asset}_C3")

    books = bases["books"]
    for word, asset in (("HELL", "T_F01_Hell"), ("HELP", "T_F01_Help")):
        save(book_title(books, word, 1), f"{asset}_C")
        save(book_title(books, word, 3), f"{asset}_C3")

    print(f"written to {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
