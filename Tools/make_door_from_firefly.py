"""Derive close/locked/blocked door SFX from the imported Firefly open clip."""

from __future__ import annotations

import math
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
SOURCE_NAME = (
    "Firefly_audio_can_you_generate_me_a_sound_effect_for_Unreal_Engi_variation2.wav"
)
TARGET_RATE = 44100


def read_wav(path: Path) -> tuple[int, list[float]]:
    with wave.open(str(path), "r") as wav:
        channels, width, rate, frames, _, _ = wav.getparams()
        raw = wav.readframes(frames)
    if width != 2:
        raise RuntimeError(f"expected 16-bit PCM, got width={width}")
    ints = struct.unpack("<" + "h" * (len(raw) // 2), raw)
    if channels == 1:
        samples = [v / 32768.0 for v in ints]
    else:
        samples = [
            (ints[i] + ints[i + 1]) * 0.5 / 32768.0 for i in range(0, len(ints) - 1, 2)
        ]
    return rate, samples


def resample(samples: list[float], src_rate: int, dst_rate: int) -> list[float]:
    if src_rate == dst_rate:
        return list(samples)
    ratio = dst_rate / src_rate
    out_n = int(round(len(samples) * ratio))
    out: list[float] = []
    last = len(samples) - 1
    for i in range(out_n):
        src = i / ratio
        lo = int(src)
        hi = min(lo + 1, last)
        t = src - lo
        out.append(samples[lo] * (1.0 - t) + samples[hi] * t)
    return out


def write_wav(path: Path, samples: list[float], peak_db: float) -> None:
    peak = max((abs(s) for s in samples), default=1.0) or 1.0
    target = 10.0 ** (peak_db / 20.0)
    scale = target / peak
    pcm = []
    for s in samples:
        v = int(round(max(-1.0, min(1.0, s * scale)) * 32767.0))
        pcm.append(max(-32767, min(32767, v)))
    with wave.open(str(path), "w") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(TARGET_RATE)
        wav.writeframes(b"".join(struct.pack("<h", s) for s in pcm))
    print(f"wrote {path.name} ({len(pcm)} frames, {len(pcm) / TARGET_RATE:.3f}s)")


def slice_sec(samples: list[float], start: float, end: float) -> list[float]:
    a = max(0, int(start * TARGET_RATE))
    b = min(len(samples), int(end * TARGET_RATE))
    return samples[a:b]


def fade(samples: list[float], in_s: float, out_s: float) -> list[float]:
    n = len(samples)
    fi = min(n, int(in_s * TARGET_RATE))
    fo = min(n, int(out_s * TARGET_RATE))
    out = list(samples)
    for i in range(fi):
        out[i] *= i / max(1, fi)
    for i in range(fo):
        out[n - 1 - i] *= i / max(1, fo)
    return out


def onepole(samples: list[float], cutoff_hz: float, highpass: bool = False) -> list[float]:
    rc = 1.0 / (2.0 * math.pi * cutoff_hz)
    dt = 1.0 / TARGET_RATE
    a = dt / (rc + dt)
    y = 0.0
    out = []
    prev = 0.0
    for x in samples:
        y += a * (x - y)
        if highpass:
            out.append(x - y)
        else:
            out.append(y)
        prev = x
    return out


def mix(a: list[float], b: list[float], b_gain: float = 1.0, offset: int = 0) -> list[float]:
    n = max(len(a), offset + len(b))
    out = [0.0] * n
    for i, v in enumerate(a):
        out[i] += v
    for i, v in enumerate(b):
        out[offset + i] += v * b_gain
    return out


def make_open(src: list[float]) -> list[float]:
    return fade(src, 0.002, 0.04)


def make_close(src: list[float]) -> list[float]:
    canvas = [0.0] * int(0.62 * TARGET_RATE)
    creak = fade(onepole(list(reversed(slice_sec(src, 0.26, 0.58))), 1800.0), 0.01, 0.05)
    thud = fade(onepole(slice_sec(src, 0.28, 0.44), 260.0), 0.002, 0.10)
    latch = fade(onepole(slice_sec(src, 0.07, 0.16), 700.0), 0.001, 0.04)
    out = mix(canvas, creak, 0.95, 0)
    out = mix(out, thud, 1.25, int(0.24 * TARGET_RATE))
    out = mix(out, latch, 0.85, int(0.34 * TARGET_RATE))
    return fade(out, 0.008, 0.08)


def make_locked(src: list[float]) -> list[float]:
    latch = fade(onepole(slice_sec(src, 0.07, 0.16), 900.0, highpass=True), 0.001, 0.03)
    out = [0.0] * int(0.52 * TARGET_RATE)
    for t, gain in ((0.012, 1.0), (0.078, 0.82), (0.138, 0.62)):
        out = mix(out, latch, gain, int(t * TARGET_RATE))
    return fade(out, 0.002, 0.04)


def make_blocked(src: list[float]) -> list[float]:
    canvas = [0.0] * int(0.48 * TARGET_RATE)
    body = fade(onepole(slice_sec(src, 0.26, 0.52), 200.0), 0.002, 0.14)
    bump = fade(onepole(slice_sec(src, 0.08, 0.20), 130.0), 0.001, 0.08)
    out = mix(canvas, body, 1.05, 0)
    out = mix(out, bump, 0.70, int(0.04 * TARGET_RATE))
    return fade(out, 0.002, 0.10)


def main() -> None:
    source = DOORS / SOURCE_NAME
    if not source.exists():
        raise SystemExit(f"missing Firefly source: {source}")

    rate, samples = read_wav(source)
    src = resample(samples, rate, TARGET_RATE)
    write_wav(DOORS / "DoorOpen.wav", make_open(src), -8.0)
    write_wav(DOORS / "DoorClose.wav", make_close(src), -7.0)
    write_wav(DOORS / "DoorLocked.wav", make_locked(src), -8.0)
    write_wav(DOORS / "DoorBlocked.wav", make_blocked(src), -7.0)


if __name__ == "__main__":
    main()
