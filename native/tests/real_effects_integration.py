#!/usr/bin/env python3
"""End-to-end physical components -> ngspice transient solve -> audible WAV regression.

No named DSP effect is involved: only R, C, D, Q and independent voltage sources.
"""
import math
import pathlib
import struct
import subprocess
import sys
import tempfile
import wave

binary = pathlib.Path(sys.argv[1]).resolve()
SAMPLE_RATE = 48000
FRAMES = 6000

def read_wav(path):
    with wave.open(str(path), "rb") as f:
        assert f.getframerate() == SAMPLE_RATE
        assert f.getnchannels() == 1
        assert f.getsampwidth() == 2
        assert f.getnframes() == FRAMES
        return struct.unpack("<" + "h" * FRAMES, f.readframes(FRAMES))

def level(data):
    return math.sqrt(sum(x*x for x in data[1000:]) / len(data[1000:]))

def render(graph, label, root, source):
    definition = root / (label + ".txt")
    definition.write_text(graph, encoding="utf-8")
    dest = root / (label + ".wav")
    proc = subprocess.run([str(binary), str(source), str(definition), str(dest)],
                          capture_output=True, text=True, timeout=90)
    assert proc.returncode == 0, (label, proc.stdout[-2000:], proc.stderr[-4000:])
    result = read_wav(dest)
    assert all(-32768 <= x <= 32767 for x in result)
    return result

def eq(c1):
    return """R1 in low 15000
C1 low 0 %s
R2 low lmix 1000
R3 lmix out 47000
C2 in midpre 22e-9
R4 midpre 0 10000
R5 midpre mid 10000
C3 mid 0 22e-9
R6 mid out 47000
C4 in high 4.7e-9
R7 high 0 10000
R8 high hmix 1000
R9 hmix out 47000
R10 out 0 100000
""" % c1

def diode(technology):
    model = {
        "GE": ("GEGEN", "D(Is=1e-6 N=1.1 Rs=1)"),
        "Si": ("DGEN", "D(Is=2.52e-9 N=1.906 Rs=0.568)"),
    }[technology]
    name, model_string = model
    return f"""R1 in out 470
D1 out 0 {name}
D2 0 out {name}
.model {name} {model_string}
"""

def ge_fuzz(voltage):
    return """V1 vneg 0 %s
C1 in b1 2.2e-6
Q1 q1c b1 0 GPNP_Q1
R1 q1c vneg 33000
Q2 q2c q1c q2e GPNP_Q2
R2 q2e 0 470
R3 q2e 0 1000
C2 q2e 0 20e-6
R4 q2c vneg 8200
R5 q2c b1 100000
C3 q2c out 10e-9
R6 out 0 500000
.model GPNP_Q1 PNP(Is=5e-6 Bf=85 Vaf=40 Eg=0.72)
.model GPNP_Q2 PNP(Is=5e-6 Bf=120 Vaf=40 Eg=0.72)
""" % voltage

with tempfile.TemporaryDirectory() as d:
    root = pathlib.Path(d)
    source = root / "source.wav"
    signal = [round(20000 * math.sin(2*math.pi*1200*i/SAMPLE_RATE)) for i in range(FRAMES)]
    with wave.open(str(source), "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        f.writeframes(struct.pack("<" + "h"*FRAMES, *signal))

    eq_a = render(eq("100e-9"), "eq_100n", root, source)
    eq_b = render(eq("10e-9"), "eq_10n", root, source)
    assert level(eq_a) > 10 and level(eq_b) > 10
    assert abs(level(eq_a) - level(eq_b)) > 5, (level(eq_a), level(eq_b))
    print("PASS: physical 3-band EQ capacitance changes actual ngspice audio RMS", level(eq_a), level(eq_b))

    di_ge = render(diode("GE"), "diode_ge", root, source)
    di_si = render(diode("Si"), "diode_si", root, source)
    assert level(di_ge) > 50 and level(di_si) > 50
    assert level(di_ge) < level(di_si)*0.85, (level(di_ge), level(di_si))
    print("PASS: germanium versus silicon diode models produce different clipping", level(di_ge), level(di_si))

    ff9 = render(ge_fuzz("-9"), "fuzz9", root, source)
    ff6 = render(ge_fuzz("-6"), "fuzz6", root, source)
    assert level(ff9) > 10 and level(ff6) > 10, (level(ff9), level(ff6))
    assert ff9 != ff6, "Changing the germanium Fuzz Face supply must change the output waveform"
    print("PASS: negative supply in real germanium Fuzz Face changes ngspice audio", level(ff9), level(ff6))
