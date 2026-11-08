#!/usr/bin/env python3
"""Actual editor -> 14 analogue graphs -> C++ parser -> ngspice -> PCM WAV.

These are educational circuit-family variants, not authenticated pedal clones.
Requires ngspice; does not replace oscilloscope/hardware measurement.
"""
import math
import pathlib
import struct
import subprocess
import sys
import tempfile
import wave

root = pathlib.Path(__file__).resolve().parents[2]
renderer = pathlib.Path(sys.argv[1]).resolve()
SAMPLE_RATE = 48000
FRAMES = 6000
EXAMPLES = [
    "mufftone", "muffdark", "rangetreble", "lpbboost", "asymclip",
    "tsfeedback", "rathard", "distplus", "bridgedt", "tilteq",
    "cockedwah", "allpass", "zenerdrive", "fuzztwo"
]

with tempfile.TemporaryDirectory(prefix="vbal-analog-") as workspace:
    tmp = pathlib.Path(workspace)
    exported = subprocess.run(
        ["node", str(root / "web" / "test_wiring.js"), "--export-analog", str(tmp)],
        cwd=root, capture_output=True, text=True, timeout=90
    )
    assert exported.returncode == 0, (exported.stdout[-3000:], exported.stderr[-3000:])

    input_file = tmp / "input.wav"
    samples = [round(20000 * math.sin(2 * math.pi * 650 * i / SAMPLE_RATE))
               for i in range(FRAMES)]
    with wave.open(str(input_file), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(SAMPLE_RATE)
        handle.writeframes(struct.pack("<" + "h" * FRAMES, *samples))

    for name in EXAMPLES:
        deck = tmp / (name + ".txt")
        assert deck.exists(), f"Missing real-editor netlist: {name}"
        output = tmp / (name + ".wav")
        proc = subprocess.run(
            [str(renderer), str(input_file), str(deck), str(output)],
            cwd=tmp, capture_output=True, text=True, timeout=120
        )
        assert proc.returncode == 0, (
            f"{name}: C++ parser / ngspice failed", proc.stdout[-2000:],
            proc.stderr[-5000:], deck.read_text()[-3000:]
        )
        with wave.open(str(output), "rb") as wave_file:
            assert wave_file.getnchannels() == 1 and wave_file.getframerate() == SAMPLE_RATE
            assert wave_file.getsampwidth() == 2 and wave_file.getnframes() == FRAMES
            result = struct.unpack("<" + "h" * FRAMES, wave_file.readframes(FRAMES))
        peak = max(abs(sample) for sample in result[1000:])
        assert peak > 2, f"{name}: silent electrical output after startup, peak={peak}"
        if name == "rangetreble":
            assert peak > 100, f"{name}: germanium bias produces near-silence, peak={peak}"
        print(f"PASS {name}: C++/ngspice/WAV, peak={peak}", flush=True)
    print("PASS: 14 real analogue editor examples rendered via ngspice without DSP macro")
