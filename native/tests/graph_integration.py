#!/usr/bin/env python3
"""Actual ngspice: editor-style circuit text -> custom topology -> output WAV."""
import pathlib,sys,tempfile,wave,math,struct,subprocess
binary=pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as root:
    root=pathlib.Path(root);src=root/"source.wav";rate=48000;frames=5000
    data=[round(16000*math.sin(2*math.pi*800*i/rate)) for i in range(frames)]
    with wave.open(str(src),"wb") as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate)
        f.writeframes(struct.pack("<"+"h"*frames,*data))
    graphs={
        "rc":"R1 in out 10000\nC1 out 0 1e-8\n",
        "custom":"R9 in middle 5000\nR10 middle out 5000\nC7 out 0 1e-8\n",
        "diode":"R1 in out 10000\nC1 out 0 1e-8\nD3 out 0 DGEN\nD4 0 out DGEN\n.model DGEN D(Is=2.52e-9 N=1.906 Rs=0.568)\n",
    }
    results={}
    for label,graph in graphs.items():
        definition=root/(label+".txt");definition.write_text(graph,encoding="utf-8")
        dest=root/(label+".wav")
        proc=subprocess.run([str(binary),str(src),str(definition),str(dest)],capture_output=True,text=True,timeout=90)
        assert proc.returncode==0,(label,proc.stdout,proc.stderr)
        with wave.open(str(dest),"rb") as output:
            assert output.getnframes()==frames and output.getframerate()==rate
            results[label]=struct.unpack("<"+"h"*frames,output.readframes(frames))
    assert max(abs(x) for x in results["rc"])>1000
    assert results["custom"]!=results["rc"],"Edited topology must change audio"
    assert max(abs(x) for x in results["diode"])<max(abs(x) for x in results["rc"])*0.95
    invalid=root/"invalid.txt";invalid.write_text("R1 in out 10000\nC1 out 0 1e-8\n.control\nquit\n")
    proc=subprocess.run([str(binary),str(src),str(invalid),str(root/"bad.wav")],capture_output=True,text=True,timeout=30)
    assert proc.returncode!=0,"Reject arbitrary SPICE control commands"
    print("PASS: custom SPICE topology and clipping WAV, forbidden commands rejected")
