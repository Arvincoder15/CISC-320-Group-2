"""Regenerate the prototype's original sound effects. Not needed to build or play."""

import math
from pathlib import Path
import random
import struct
import wave


RATE = 24000
OUT = Path(__file__).resolve().parents[1] / "assets" / "audio"


def note(start, length, first, last, gain, harmonic=0, noise=0):
    return start, length, first, last, gain, harmonic, noise


SOUNDS = {
    "select": [note(0, .076, 540, 735, .22, .18), note(.012, .076, 1140, 980, .06)],
    "fire_jump": [note(0, .145, 145, 445, .29, .34, .11),
                  note(.018, .09, 88, 247, .10, .20)],
    "water_jump": [note(0, .103, 545, 1015, .27), note(.064, .115, 755, 1280, .17, .12)],
    "lever": [note(0, .041, 920, 300, .30, .14, .55),
              note(.036, .078, 245, 112, .28, .38, .22)],
    "gate": [note(0, .18, 103, 155, .24, .32, .12), note(.095, .21, 330, 333, .23, .15),
             note(.175, .24, 494, 495, .22, .18)],
    "defeat": [note(0, .17, 393, 332, .28, .21), note(.135, .19, 311, 239, .28, .20),
               note(.28, .29, 199, 98, .31, .22)],
    "complete": [note(t, d, f, f, .28, .18)
                 for t, d, f in [(0, .16, 523.25), (.105, .16, 659.25),
                                 (.210, .19, 783.99), (.335, .31, 1046.5)]],
}


def render(name, notes):
    rng = random.Random(name)
    samples = [0.] * (math.ceil(max(n[0] + n[1] for n in notes) * RATE) + 1)
    for start, length, low, high, gain, harmonic, noise in notes:
        for i in range(round(RATE * length)):
            t = i / RATE
            phase = math.tau * (low * t + .5 * (high - low) * t * t / length)
            envelope = min(1., t / .005) * max(0., 1. - t / length) ** 1.45
            tone = math.sin(phase) + harmonic * math.sin(2 * phase)
            samples[round(start * RATE) + i] += gain * envelope * (tone + noise * rng.uniform(-1, 1))
    with wave.open(str(OUT / (name + ".wav")), "wb") as wav:
        wav.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
        wav.writeframes(b"".join(struct.pack("<h", round(32767 * s)) for s in samples))


if __name__ == "__main__":
    OUT.mkdir(parents=True, exist_ok=True)
    for effect, notes in SOUNDS.items():
        render(effect, notes)
