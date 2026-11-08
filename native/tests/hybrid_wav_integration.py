#!/usr/bin/env python3
"""End-to-end VBAL hybrid WAV (real 16/8-pin nets, R-C and supply).

This is the native VBAL MNA mixed-signal solver, NOT a standalone ngspice run.
"""
import math
import pathlib
import struct
import subprocess
import sys
import tempfile
import wave

binary = pathlib.Path(sys.argv[1]).resolve()
rate = 48000
frames = 14000

def pt2399(resistance, supply=5):
    return f"""V1 vcc 0 {supply}
R1 in sig 1000
R2 sig ref 100000
R3 vco 0 {resistance}
R4 out 0 100000
C1 wet out 1e-6
X1 vcc ref 0 0 clk vco cc1 cc0 op1out op1in op2in op2out lpf2in wet lpf1out sig PT2399_VBAL
"""

def bbd(resistance, supply=-15):
    return f"""V1 vneg 0 {supply}
R1 vneg bias 100000
R2 bias 0 100000
C1 in bias 1e-6
R3 osc 0 {resistance}
C2 oscCap 0 100e-12
C3 bbdOut out 1e-6
R5 out 0 100000
R6 oscIn 0 22000
C4 bbdOut2 out 1e-6
XCLK1 0 cp1 vneg cp2 oscCap osc oscIn vgg MN3101_VBAL
XBB1 0 cp1 bias vgg vneg cp2 bbdOut bbdOut2 MN3007_VBAL
"""

with tempfile.TemporaryDirectory() as tmp:
    root = pathlib.Path(tmp)
    sample = root / "guitar.wav"
    waveform = [round(11000 * math.sin(2*math.pi*920*i/rate)) if 1000 <= i < 3000 else 0 for i in range(frames)]
    with wave.open(str(sample),"wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(rate)
        f.writeframes(struct.pack("<"+"h"*frames,*waveform))

    def render(name, circuit, success=True):
        deck = root / (name + ".txt")
        deck.write_text(circuit,encoding="utf-8")
        dest = root / (name + ".wav")
        process = subprocess.run([str(binary),str(sample),str(deck),str(dest)],
                                 capture_output=True,text=True,timeout=120)
        if not success:
            assert process.returncode != 0, (name,process.stdout,process.stderr)
            return []
        assert process.returncode == 0,(name,process.stdout,process.stderr)
        assert "hybrid" in process.stdout.lower(),process.stdout
        with wave.open(str(dest),"rb") as f:
            assert f.getnframes() == frames and f.getframerate() == rate
            return struct.unpack("<"+"h"*frames,f.readframes(frames))

    fast = render("pt_fast",pt2399(2000))
    slow = render("pt_slow",pt2399(10500))
    assert fast != slow, "PT2399 R6 must control audible delay time"
    assert max(abs(x) for x in fast[3500:6000]) > 40,"PT2399 fast delay has no energy"
    assert max(abs(x) for x in slow[8500:]) > 40,"PT2399 slow delay has no energy"
    render("pt_no_control",pt2399(2000).replace("R3 vco 0 2000\n",""),False)
    bbd_fast = render("bbd_fast",bbd(22000))
    bbd_slow = render("bbd_slow",bbd(39000))
    print("BBD diagnostics:", "fast_minmax",min(bbd_fast),max(bbd_fast), "slow_minmax",min(bbd_slow),max(bbd_slow), "fast_tail",bbd_fast[-8:], "slow_tail",bbd_slow[-8:],"first_diff",next((i for i,(a,b) in enumerate(zip(bbd_fast,bbd_slow)) if a!=b),-1),flush=True)
    assert bbd_fast != bbd_slow,"MN3101 R-C clock must affect BBD sound"
    assert max(abs(x) for x in bbd_fast) > 20,"BBD charge output is silent"
    render("bbd_no_clock",bbd(22000).replace("XCLK1 0 cp1 vneg cp2 oscCap osc oscIn vgg MN3101_VBAL\n",""),False)
    print("PASS: PT2399/MN3007 hybrid audio from physical DIP nets; resistor-controlled delay and clock, no missing pin controls")
