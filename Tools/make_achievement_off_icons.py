"""Make the locked ("_off") variant of every achievement icon from its "_on" file.

Steam wants two 256x256 icons per achievement. The unlocked one is authored;
the locked one is the same picture desaturated and darkened, which is what the
existing ACH_SPOT_LOOPNUMBER pair already does. Run after dropping a new
*_on.png into Marketing/Steam/Achievements:

    py -3 Tools/make_achievement_off_icons.py            # only missing _off files
    py -3 Tools/make_achievement_off_icons.py --force    # regenerate all

Requires Pillow (py -3 -m pip install pillow).
"""
from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageEnhance, ImageOps

ICON_DIR = Path(__file__).resolve().parent.parent / "Marketing" / "Steam" / "Achievements"
SIZE = (256, 256)
BRIGHTNESS = 0.62   # matches the shipped LoopNumber pair
CONTRAST = 0.95


def make_off(on_path: Path, off_path: Path) -> None:
    img = Image.open(on_path).convert("RGB")
    if img.size != SIZE:
        img = img.resize(SIZE, Image.LANCZOS)
    grey = ImageOps.grayscale(img).convert("RGB")
    grey = ImageEnhance.Brightness(grey).enhance(BRIGHTNESS)
    grey = ImageEnhance.Contrast(grey).enhance(CONTRAST)
    grey.save(off_path, "PNG", optimize=True)


def main(argv: list[str]) -> int:
    force = "--force" in argv
    if not ICON_DIR.is_dir():
        print(f"missing folder: {ICON_DIR}")
        return 1

    made = 0
    for on_path in sorted(ICON_DIR.glob("*_on.png")):
        off_path = on_path.with_name(on_path.name[:-len("_on.png")] + "_off.png")
        if off_path.exists() and not force:
            continue
        make_off(on_path, off_path)
        made += 1
        print(f"{off_path.name}")

    # Steam also refuses anything that is not exactly 256x256.
    for path in sorted(ICON_DIR.glob("*.png")):
        with Image.open(path) as img:
            if img.size != SIZE:
                print(f"WARNING {path.name} is {img.size[0]}x{img.size[1]}, Steam wants 256x256")

    print(f"{made} locked icon(s) written")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
