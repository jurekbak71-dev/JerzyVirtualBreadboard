#!/usr/bin/env python3
"""Integration: real ngspice antiphase-diode audio limiter vs linear RC."""
import math
import pathlib
import struct
import subprocess
import sys
import tempfile
import wave

binary = pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as directory:
    root=pathlib.Path(directory)
    source=root/"input.wav"
    rate=48000
    frames=5000
    values=[int(29000*math.sin(2*math.pi*650*i/rate)) for i in range(frames)]
    with wave.open(str(source),"wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(rate)
        wav.writeframes(struct.pack("<"+"h"*frames,*values))
    results={}
    for mode in ("rc","diode"):
        target=root/(mode+".wav")
        proc=subprocess.run([str(binary),str(source),str(target),"10000,1e-8",mode],
                            capture_output=True,text=True,timeout=90)
        if proc.returncode:
            raise AssertionError(f"{mode} solver error:\n{proc.stdout}\n{proc.stderr}")
        with wave.open(str(target),"rb") as wav:
            assert wav.getframerate()==rate and wav.getnframes()==frames
            results[mode]=struct.unpack("<"+"h"*frames,wav.readframes(frames))
    linear=max(map(abs,results["rc"]))
    clipped=max(map(abs,results["diode"]))
    assert linear>10000, f"RC output unexpectedly quiet: {linear}"
    assert 1000<clipped<linear*0.92, f"Diodes did not produce expected attenuation: linear={linear}, clipped={clipped}"
    print(f"PASS: real diode clipper vs RC; peak RC={linear}, diode={clipped}")
