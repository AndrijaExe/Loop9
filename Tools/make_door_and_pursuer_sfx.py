"""Synthesize door SFX and a looping pursuer tension bed (mono 44.1 kHz WAV)."""

from __future__ import annotations

import math
import random
import struct
import wave
from pathlib import Path

SAMPLE_RATE = 44100
SOUND_ROOT = (
    Path(__file__).resolve().parents[1]
    / "Content"
    / "MyStuff"
    / "Sound"
)


def peak_normalize(samples: list[float], peak_db: float) -> list[int]:
    peak = max(abs(s) for s in samples) or 1.0
    target = 10.0 ** (peak_db / 20.0)
    scale = target / peak
    out = []
    for s in samples:
        v = int(round(max(-1.0, min(1.0, s * scale)) * 32767.0))
        out.append(max(-32767, min(32767, v)))
    return out


def write_wav(path: Path, samples: list[float], peak_db: float) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    pcm = peak_normalize(samples, peak_db)
    with wave.open(str(path), "w") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(b"".join(struct.pack("<h", s) for s in pcm))
    print(f"wrote {path} ({len(pcm)} frames)")


def add_click(
    samples: list[float],
    rng: random.Random,
    center: float,
    amp: float,
    high_hz: float,
    decay: float,
    thump_hz: float = 140.0,
) -> None:
    n = len(samples)
    for i in range(n):
        t = i / SAMPLE_RATE - center
        if t < 0:
            continue
        env = math.exp(-t * decay)
        if env < 0.0004:
            continue
        noise = rng.uniform(-1.0, 1.0)
        tone = math.sin(2.0 * math.pi * high_hz * t)
        thump = math.sin(2.0 * math.pi * thump_hz * t)
        samples[i] += amp * env * (0.45 * noise + 0.25 * tone + 0.30 * thump)


def synth_open() -> list[float]:
    n = int(SAMPLE_RATE * 0.62)
    samples = [0.0] * n
    rng = random.Random(11)
    add_click(samples, rng, 0.012, 0.35, 2100.0, 70.0, 90.0)
    for i in range(n):
        t = i / SAMPLE_RATE
        env = math.exp(-t * 4.2) * (0.35 + 0.65 * math.sin(math.pi * min(1.0, t / 0.55)))
        creak = math.sin(2.0 * math.pi * (190.0 + 40.0 * t) * t)
        wood = rng.uniform(-1.0, 1.0) * math.exp(-t * 6.0)
        samples[i] += env * (0.55 * creak + 0.18 * wood)
    return samples


def synth_close() -> list[float]:
    n = int(SAMPLE_RATE * 0.38)
    samples = [0.0] * n
    rng = random.Random(17)
    add_click(samples, rng, 0.018, 0.55, 900.0, 45.0, 85.0)
    add_click(samples, rng, 0.055, 0.28, 2400.0, 90.0, 160.0)
    return samples


def synth_locked() -> list[float]:
    n = int(SAMPLE_RATE * 0.48)
    samples = [0.0] * n
    rng = random.Random(23)
    add_click(samples, rng, 0.010, 0.42, 1800.0, 80.0, 220.0)
    add_click(samples, rng, 0.072, 0.38, 1650.0, 85.0, 200.0)
    add_click(samples, rng, 0.128, 0.30, 1950.0, 95.0, 240.0)
    return samples


def synth_blocked() -> list[float]:
    n = int(SAMPLE_RATE * 0.42)
    samples = [0.0] * n
    rng = random.Random(29)
    add_click(samples, rng, 0.014, 0.62, 420.0, 28.0, 62.0)
    add_click(samples, rng, 0.090, 0.22, 1100.0, 55.0, 90.0)
    return samples


def synth_tension() -> list[float]:
    duration = 8.0
    n = int(SAMPLE_RATE * duration)
    samples = [0.0] * n
    rng = random.Random(41)
    fade = int(SAMPLE_RATE * 0.28)
    for i in range(n):
        t = i / SAMPLE_RATE
        lfo = 0.72 + 0.28 * math.sin(2.0 * math.pi * 0.22 * t)
        drone = (
            0.55 * math.sin(2.0 * math.pi * 46.0 * t)
            + 0.32 * math.sin(2.0 * math.pi * 69.5 * t)
            + 0.18 * math.sin(2.0 * math.pi * 92.5 * t)
        )
        pulse = 0.12 * math.sin(2.0 * math.pi * 1.15 * t) * math.sin(2.0 * math.pi * 55.0 * t)
        noise = rng.uniform(-1.0, 1.0) * 0.04 * (0.5 + 0.5 * math.sin(2.0 * math.pi * 0.08 * t))
        samples[i] = lfo * (drone + pulse + noise)
    for i in range(fade):
        w = i / fade
        samples[i] *= w
        samples[n - 1 - i] *= w
    # Equal-power-ish join so the loop is usable after import sets Looping.
    for i in range(fade):
        w = i / fade
        samples[i] += samples[n - fade + i] * (1.0 - w)
        samples[n - fade + i] *= w
    return samples


def main() -> None:
    doors = SOUND_ROOT / "Doors"
    write_wav(doors / "DoorOpen.wav", synth_open(), -8.0)
    write_wav(doors / "DoorClose.wav", synth_close(), -6.0)
    write_wav(doors / "DoorLocked.wav", synth_locked(), -7.0)
    write_wav(doors / "DoorBlocked.wav", synth_blocked(), -6.0)
    write_wav(SOUND_ROOT / "Pursuer" / "PursuerTensionLoop.wav", synth_tension(), -12.0)


if __name__ == "__main__":
    main()
