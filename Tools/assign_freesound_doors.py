"""Copy Freesound door clips into the C++ load slots and trim the slam."""

from __future__ import annotations

import shutil
import struct
import wave
from pathlib import Path

DOORS = (
    Path(__file__).resolve().parents[1]
    / "Content"
    / "MyStuff"
    / "Sound"
    / "Doors"
)
CLOSE_SRC = DOORS / "431118__inspectorj__door-front-closing-a.wav"
LOCKED_SRC = DOORS / "321087__benjaminnelan__door-locked.wav"
BLOCKED_SRC = DOORS / "475850__alfonsseelen__car_door_slam_flat_block_overvecht.wav"
BLOCKED_SECONDS = 0.4
FADE_OUT_SECONDS = 0.02


def trim_wav(src: Path, dst: Path, seconds: float) -> None:
    with wave.open(str(src), "r") as wav:
        channels, width, rate, frames, _, _ = wav.getparams()
        keep = min(frames, int(round(seconds * rate)))
        raw = wav.readframes(keep)

    fade = min(keep, int(round(FADE_OUT_SECONDS * rate)))
    if width == 2 and fade > 1:
        sample_count = keep * channels
        samples = list(struct.unpack("<" + "h" * sample_count, raw))
        for i in range(fade):
            gain = 1.0 - (i / (fade - 1))
            idx = (keep - fade + i) * channels
            for c in range(channels):
                samples[idx + c] = int(samples[idx + c] * gain)
        raw = b"".join(struct.pack("<h", s) for s in samples)

    with wave.open(str(dst), "w") as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(width)
        wav.setframerate(rate)
        wav.writeframes(raw)

    print(f"wrote {dst.name} ({keep / rate:.3f}s)")


def main() -> None:
    for src in (CLOSE_SRC, LOCKED_SRC, BLOCKED_SRC):
        if not src.exists():
            raise SystemExit(f"missing {src.name}")

    shutil.copyfile(CLOSE_SRC, DOORS / "DoorClose.wav")
    print(f"copied {CLOSE_SRC.name} -> DoorClose.wav")
    shutil.copyfile(LOCKED_SRC, DOORS / "DoorLocked.wav")
    print(f"copied {LOCKED_SRC.name} -> DoorLocked.wav")
    trim_wav(BLOCKED_SRC, DOORS / "DoorBlocked.wav", BLOCKED_SECONDS)


if __name__ == "__main__":
    main()
