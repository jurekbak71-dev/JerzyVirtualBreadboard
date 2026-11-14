"""Byte-reproducibility, physical-data claims, and header consistency tests."""
import hashlib
import importlib.util
from pathlib import Path
import re
import struct
import tempfile
import unittest
import wave

HERE=Path(__file__).resolve().parent
ROOT=HERE.parent
spec=importlib.util.spec_from_file_location('GenerateFactoryIR',HERE/'GenerateFactoryIR.py')
mod=importlib.util.module_from_spec(spec)
import sys
sys.modules['GenerateFactoryIR']=mod
spec.loader.exec_module(mod)

class FactoryIRTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.outputs=mod.generated_files()

    def test_exact_names_and_duration(self):
        self.assertEqual(tuple(room.name for room in mod.ROOMS),
                         ('Booth','Small Room','Studio','Plate','Hall'))
        self.assertEqual(tuple(round(r.seconds,2) for r in mod.ROOMS),
                         (.45,.90,1.45,2.30,3.70))

    def test_generated_artifacts_unchanged(self):
        for rel,data in self.outputs.items():
            with self.subTest(path=rel):
                self.assertEqual((ROOT/rel).read_bytes(),data)
        self.assertTrue(mod.write_or_check(ROOT,check=True))

    def test_distinct_nonempty_wavs(self):
        signatures=set()
        for room in mod.ROOMS:
            data=self.outputs['assets/ir/'+room.symbol+'.wav']
            import io
            with wave.open(io.BytesIO(data),'rb') as wav:
                self.assertEqual((wav.getframerate(),wav.getnchannels(),wav.getsampwidth()),(48000,1,2))
                self.assertEqual(wav.getnframes(),round(room.seconds*48000))
                pcm=struct.unpack('<'+'h'*wav.getnframes(),wav.readframes(wav.getnframes()))
                self.assertTrue(all(-32768<=x<=32767 for x in pcm))
                self.assertGreater(sum(x!=0 for x in pcm),100)
                self.assertEqual(pcm[0],24576)
            signatures.add(hashlib.sha256(data).digest())
        self.assertEqual(len(signatures),5)

    def test_header_exactly_matches_pcm(self):
        source=self.outputs['Source/FactoryRooms.h'].decode()
        self.assertIn('original synthetic signals',source)
        self.assertIn('CC0-1.0',source)
        for room in mod.ROOMS:
            pcm=mod.synthesize(room)
            pattern=r'inline constexpr Tap k'+room.symbol+r'\[\] = \{(.*?)\n\};'
            match=re.search(pattern,source,re.S)
            self.assertIsNotNone(match,room.name)
            taps=[(int(a),int(b)) for a,b in re.findall(r'\{(\d+),\s*(-?\d+)\}',match.group(1))]
            recovered=[0]*len(pcm)
            for index,value in taps:
                self.assertEqual(recovered[index],0)
                recovered[index]=value
            self.assertEqual(tuple(recovered),pcm)

    def test_check_fails_on_modified_bytes(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            for rel,data in self.outputs.items():
                path=root/rel
                path.parent.mkdir(parents=True,exist_ok=True)
                path.write_bytes(data)
            self.assertTrue(mod.write_or_check(root,check=True))
            damaged=root/'assets/ir/Hall.wav'
            damaged.write_bytes(damaged.read_bytes()[:-2])
            self.assertFalse(mod.write_or_check(root,check=True))

if __name__=='__main__': unittest.main()
