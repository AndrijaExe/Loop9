"""Synthesize one keyboard keypress (mono 44.1 kHz WAV) for the ending typing effect.

One asset is enough: the ending widget randomises pitch per keystroke, which is
what keeps a line of dialogue from sounding like a machine gun on one note.
"""

from __future__ import annotations

import math
import random
import struct
import wave
from pathlib import Path

SAMPLE_RATE = 44100
DURATION = 0.075
OUT_PATH = (
    Path(__file__).resolve().parents[1]
    / "Content"
    / "MyStuff"
    / "Sound"
    / "UI"
    / "TypewriterKey.wav"
)


def synth() -> list[float]:
    n = int(SAMPLE_RATE * DURATION)
    samples = [0.0] * n
    rng = random.Random(1963)

    for i in range(n):
        t = i / SAMPLE_RATE

        # Contact click: broadband, gone in a few milliseconds.
        click_env = math.exp(-t * 420.0)
        click = rng.uniform(-1.0, 1.0) * click_env * 0.5

        # Keycap bottoming out on the plate.
        thock_env = math.exp(-t * 95.0)
        thock = (
            math.sin(2.0 * math.pi * 190.0 * t) * 0.6
            + math.sin(2.0 * math.pi * 320.0 * t) * 0.25
        ) * thock_env * 0.42

        # Thin metallic ring so it reads as a machine, not a desk tap.
        ring_env = math.exp(-t * 260.0)
        ring = math.sin(2.0 * math.pi * 3100.0 * t) * ring_env * 0.14

        samples[i] = click + thock + ring

    return samples


def peak_normalize(samples: list[float], peak_db: float = -10.0) -> list[int]:
    peak = max(abs(s) for s in samples) or 1.0
    target = 10.0 ** (peak_db / 20.0)
    scale = target / peak
    out = []
    for s in samples:
        v = int(round(max(-1.0, min(1.0, s * scale)) * 32767.0))
        out.append(max(-32767, min(32767, v)))
    return out


def main() -> None:
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    pcm = peak_normalize(synth())
    with wave.open(str(OUT_PATH), "w") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(b"".join(struct.pack("<h", s) for s in pcm))
    print(f"wrote {OUT_PATH} ({len(pcm)} frames)")


if __name__ == "__main__":
    main()
