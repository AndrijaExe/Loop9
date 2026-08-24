"""Synthesize a short plastic flashlight switch click (mono 44.1 kHz WAV)."""

from __future__ import annotations

import math
import random
import struct
import wave
from pathlib import Path

SAMPLE_RATE = 44100
DURATION = 0.11
OUT_PATH = (
    Path(__file__).resolve().parents[1]
    / "Content"
    / "MyStuff"
    / "Sound"
    / "Flashlight"
    / "FlashlightToggle.wav"
)


def synth() -> list[float]:
    n = int(SAMPLE_RATE * DURATION)
    samples = [0.0] * n
    rng = random.Random(9)

    def click(center: float, amp: float, high_hz: float, decay: float) -> None:
        width = int(SAMPLE_RATE * 0.004)
        start = max(0, int(center * SAMPLE_RATE) - width)
        for i in range(start, n):
            t = i / SAMPLE_RATE - center
            if t < 0:
                continue
            env = math.exp(-t * decay)
            if env < 0.0005:
                break
            noise = rng.uniform(-1.0, 1.0)
            tone = math.sin(2.0 * math.pi * high_hz * t)
            thump = math.sin(2.0 * math.pi * 180.0 * t)
            samples[i] += amp * env * (0.55 * noise + 0.30 * tone + 0.15 * thump)

    # Two latch transients, a few milliseconds apart.
    click(0.008, 0.55, 2650.0, 90.0)
    click(0.028, 0.38, 1900.0, 70.0)
    return samples


def peak_normalize(samples: list[float], peak_db: float = -8.0) -> list[int]:
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
