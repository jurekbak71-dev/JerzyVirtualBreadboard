#!/usr/bin/env python3
"""Check ngspice XSPICE filesource for audio sequences longer than inline PWL limit."""
import math
import pathlib
import subprocess
import tempfile
with tempfile.TemporaryDirectory() as tmp:
    root=pathlib.Path(tmp)
    rate=48000
    count=5000
    with (root/"signal.dat").open("w",encoding="ascii") as out:
        for i in range(count+1):
            out.write(f"{i/rate:.17g} {0.5*math.sin(2*math.pi*500*i/rate):.17g}\n")
    netlist="""* VBAL ngspice filesource test
ASrc %vd([in 0]) audiosrc
.model audiosrc filesource (file="signal.dat" amploffset=[0] amplscale=[1]
+ timeoffset=0 timescale=1 timerelative=false amplstep=false)
R1 in out 10k
C1 out 10n
.control
set wr_singlescale
set wr_vecnames
tran 2.0833333333333333e-5 0.10416666666666667
wrdata output.dat v(out)
.endc
.end
"""
    (root/"test.cir").write_text(netlist,encoding="ascii")
    proc=subprocess.run(["ngspice","-b","test.cir"],cwd=root,capture_output=True,text=True,timeout=90)
    if proc.returncode:
        raise AssertionError(f"ngspice filesource failed\n{proc.stdout}\n{proc.stderr}")
    result=root/"output.dat"
    assert result.is_file() and result.stat().st_size>1000, f"Missing output\n{proc.stdout}\n{proc.stderr}"
    values=[]
    for line in result.read_text().splitlines():
        try: t,v=map(float,line.split()[:2])
        except (ValueError,IndexError): continue
        values.append(v)
    assert len(values)>1000 and max(map(abs,values))>0.01, f"Silent or truncated filesource: {len(values)}"
    print(f"PASS: XSPICE filesource simulated {count} input frames; {len(values)} output samples")
