"""Render Elemental Co-op's original eight-bar background loop as PCM WAV."""

import math
from pathlib import Path
import struct
import wave

RATE = 16000
BEAT = 8000  # 120 bpm
LOOP_SAMPLES = 32 * BEAT
OUTPUT = Path(__file__).resolve().parents[1] / "assets" / "audio" / "background.wav"

# A minor, F major, C major, G major. Each bar lists MIDI notes for bass,
# arpeggio and melody. Melody rests make room for the gameplay cues.
CHORDS = [
    (45, (57, 60, 64, 67), (76, None, 72, 74)),
    (41, (53, 57, 60, 64), (77, 76, None, 72)),
    (48, (55, 60, 64, 67), (79, None, 76, 74)),
    (43, (55, 59, 62, 67), (74, 71, None, 72)),
    (45, (57, 60, 64, 69), (76, None, 81, 79)),
    (41, (53, 57, 60, 65), (77, 76, None, 72)),
    (48, (55, 60, 64, 67), (79, None, 76, 72)),
    (43, (55, 59, 62, 67), (74, 71, 74, None)),
]


def add_note(samples, start, beats, midi, amplitude, overtone):
    duration = round(BEAT * beats)
    radians_per_sample = math.tau * 440 * 2 ** ((midi - 69) / 12) / RATE
    for i in range(duration):
        phase = i * radians_per_sample
        attack = min(1, i / (RATE * 0.018))
        release = (1 - i / duration) ** 1.25
        wave_sample = math.sin(phase) + overtone * math.sin(2 * phase)
        samples[(start + i) % LOOP_SAMPLES] += amplitude * attack * release * wave_sample


def main():
    samples = [0.0] * LOOP_SAMPLES
    for bar, (bass, chord, melody) in enumerate(CHORDS):
        start = bar * 4 * BEAT
        for beat in (0, 2):
            add_note(samples, start + beat * BEAT, 1.85, bass, 0.11, 0.18)
        for step, note in enumerate((0, 1, 2, 3, 2, 1, 3, 2)):
            add_note(samples, start + step * (BEAT // 2), 0.46, chord[note], 0.08, 0.35)
        for beat, note in enumerate(melody):
            if note is not None:
                add_note(samples, start + beat * BEAT, 0.81, note, 0.12, 0.23)

    values = [round(sample * 32767) for sample in samples]
    with wave.open(str(OUTPUT), "wb") as wav:
        wav.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
        wav.writeframes(struct.pack(f"<{len(values)}h", *values))


if __name__ == "__main__":
    main()
