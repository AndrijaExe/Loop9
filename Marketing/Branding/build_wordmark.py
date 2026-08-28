"""Outline the SMENA wordmark (variant 02) into font-independent SVG paths."""

import os

from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.ttLib import TTFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = os.path.join(HERE, "ShareTechMono-Regular.ttf")
OUT = os.path.join(HERE, "out")

WORD = "SMENA"
CURSOR = "_"
TRACKING_EM = 0.40  # matches letter-spacing: .40em in the mockup
PAD_EM = 0.10

INK = "#F2F0EC"
PHOSPHOR = "#7CE0A0"


def glyph_data():
    font = TTFont(FONT)
    glyphs = font.getGlyphSet()
    cmap = font.getBestCmap()
    upem = font["head"].unitsPerEm

    out = []
    for ch in WORD + CURSOR:
        glyph = glyphs[cmap[ord(ch)]]

        path_pen = SVGPathPen(glyphs, ntos=lambda v: f"{v:.0f}")
        glyph.draw(path_pen)

        bounds_pen = BoundsPen(glyphs)
        glyph.draw(bounds_pen)

        out.append(
            {
                "char": ch,
                "d": path_pen.getCommands(),
                "advance": glyph.width,
                "bounds": bounds_pen.bounds,
            }
        )
    return out, upem


def place(chars, upem):
    """Walk the string, returning each glyph with its pen position and the ink bbox."""
    tracking = TRACKING_EM * upem
    x = 0.0
    placed = []
    x_min = y_min = float("inf")
    x_max = y_max = float("-inf")

    for i, g in enumerate(chars):
        placed.append({**g, "x": x})
        if g["bounds"]:
            gx0, gy0, gx1, gy1 = g["bounds"]
            x_min = min(x_min, x + gx0)
            x_max = max(x_max, x + gx1)
            y_min = min(y_min, gy0)
            y_max = max(y_max, gy1)
        x += g["advance"]
        if i < len(chars) - 1:
            x += tracking

    return placed, (x_min, y_min, x_max, y_max)


def build_svg(placed, ink_bbox, upem, *, mono_fill):
    """Emit an SVG whose viewBox hugs the ink, so it drops into any layout cleanly."""
    pad = PAD_EM * upem
    x_min, y_min, x_max, y_max = ink_bbox
    width = (x_max - x_min) + 2 * pad
    height = (y_max - y_min) + 2 * pad

    # Font space is y-up, SVG is y-down: flip inside the group and offset the
    # baseline so the ink lands inside the viewBox.
    tx = pad - x_min
    ty = pad + y_max

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width:.0f} {height:.0f}" '
        f'width="{width / upem * 100:.1f}" height="{height / upem * 100:.1f}" '
        f'role="img" aria-label="SMENA">',
        "  <title>SMENA</title>",
        f'  <g transform="translate({tx:.0f} {ty:.0f}) scale(1 -1)">',
    ]

    for g in placed:
        if not g["d"]:
            continue
        if mono_fill:
            fill = mono_fill
        else:
            fill = PHOSPHOR if g["char"] == CURSOR else INK
        parts.append(
            f'    <path fill="{fill}" transform="translate({g["x"]:.0f} 0)" d="{g["d"]}"/>'
        )

    parts += ["  </g>", "</svg>", ""]
    return "\n".join(parts)


def main():
    os.makedirs(OUT, exist_ok=True)
    chars, upem = glyph_data()
    placed, ink_bbox = place(chars, upem)

    variants = (
        ("smena-wordmark.svg", None),
        ("smena-wordmark-currentcolor.svg", "currentColor"),
        ("smena-wordmark-white.svg", INK),
        ("smena-wordmark-black.svg", "#0A0A0B"),
    )
    for name, mono_fill in variants:
        path = os.path.join(OUT, name)
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(build_svg(placed, ink_bbox, upem, mono_fill=mono_fill))
        print(f"wrote {path}")

    x0, y0, x1, y1 = ink_bbox
    print(f"upem={upem}  ink={x1 - x0:.0f}x{y1 - y0:.0f}  aspect={(x1 - x0) / (y1 - y0):.3f}")
    print("glyphs:", ", ".join(f"{g['char']}@{g['x']:.0f}" for g in placed))


if __name__ == "__main__":
    main()
