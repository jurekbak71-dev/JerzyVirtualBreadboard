"""Audit/version the rendered assets. --write-lock requires Pillow (authoring only)."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
ASSETS=ROOT/'assets/gui/rendered-v1'
LOCK=ROOT/'tools/gui-rendered-v1.lock.json'

def verify():
    lock=json.loads(LOCK.read_text())
    actual={p.relative_to(ASSETS).as_posix() for p in ASSETS.rglob('*.png')}
    if actual!=set(lock['files']):raise ValueError('Rendered inventory differs from lock')
    for name,info in lock['files'].items():
        data=(ASSETS/name).read_bytes()
        if hashlib.sha256(data).hexdigest()!=info['sha256']:raise ValueError('Rendered SHA256 mismatch: '+name)
        if data[:8]!=b'\x89PNG\r\n\x1a\n' or tuple(struct.unpack('>II',data[16:24]))!=tuple(info['size']):raise ValueError('PNG dimensions invalid: '+name)
    print(f"Verified {len(actual)} rendered PNGs, SHA256 and dimensions")

def write_lock():
    from PIL import Image
    files={}
    for p in sorted(ASSETS.rglob('*.png')):
        with Image.open(p) as image:
            image.load()
            files[p.relative_to(ASSETS).as_posix()]={'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':list(image.size),'mode':image.mode}
    report=[]
    for style in ('steel_large','steel_warm','black_anodized','steel_bright','chrome_small','black_small','brass_small'):
        p=ASSETS/(style+'_index_128.png')
        with Image.open(p) as im:
            if im.size!=(1536,768):raise ValueError('Index atlas dimensions: '+style)
            frames=[]
            for f in range(128):
                x=f%16*96;y=f//16*96;frame=im.crop((x,y,x+96,y+96))
                if frame.getchannel('A').getbbox() is None:raise ValueError(f'Empty frame {style}:{f}')
                frames.append(hashlib.sha256(frame.tobytes()).hexdigest())
            if len(set(frames))!=128:raise ValueError('Repeated index frames: '+style)
            report.append({'style':style,'frames':128,'distinct':128,'bodyIsSeparate':True})
    LOCK.write_text(json.dumps({'version':'rendered-v1','source':'tools/blender/render_assets.py and render_controls.py','files':files,'knobAudit':report},indent=2)+'\n')
    print(f'Wrote versioned lock for {len(files)} fully decoded PNGs and seven 128-frame index atlases')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--write-lock',action='store_true');args=parser.parse_args()
    if args.write_lock:write_lock()
    verify()
