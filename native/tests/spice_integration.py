#!/usr/bin/env python3
"""End-to-end test: WAV -> actual ngspice -> WAV. Runs in GitHub Actions Ubuntu."""
import math
import pathlib
import struct
import subprocess
import sys
import tempfile
import wave

binary = pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as directory:
    source = pathlib.Path(directory) / "source.wav"
    output = pathlib.Path(directory) / "processed.wav"
    rate, frames = 48000, 5000
    # A stable test tone below RC cutoff; output must be finite and audible.
    signal = [round(11000 * math.sin(2 * math.pi * 500 * i / rate)) for i in range(frames)]
    with wave.open(str(source), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(rate)
        wav.writeframes(struct.pack("<" + "h" * frames, *signal))

    completed = subprocess.run([str(binary), str(source), str(output), "10000,1e-8"],
                               text=True, capture_output=True, timeout=90)
    if completed.returncode:
        raise AssertionError(f"ngspice render failed: {completed.stdout}\n{completed.stderr}")
    with wave.open(str(output), "rb") as wav:
        assert wav.getnchannels() == 1
        assert wav.getframerate() == rate
        assert wav.getnframes() == frames
        samples = struct.unpack("<" + "h" * frames, wav.readframes(frames))
    assert max(abs(s) for s in samples) > 1000, "Unexpectedly silent output"
    assert samples != tuple(signal), "Solver must alter the waveform"
    print("PASS: actual ngspice WAV render")
