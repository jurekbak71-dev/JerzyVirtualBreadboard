import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

SPEC = importlib.util.spec_from_file_location('import_assets', Path(__file__).parents[1] / 'tools/import_gui_assets.py')

class ImportTests(unittest.TestCase):
    def test_rejects_wrong_digest_without_writing(self):
        module = importlib.util.module_from_spec(SPEC)
        SPEC.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as d:
            archive = Path(d) / 'bad.zip'
            archive.write_bytes(b'not the approved archive')
            target = Path(d) / 'out'
            with self.assertRaisesRegex(ValueError, 'SHA256'):
                module.import_assets(archive, target)
            self.assertFalse(target.exists())

    def test_safe_relative_paths(self):
        module = importlib.util.module_from_spec(SPEC)
        SPEC.loader.exec_module(module)
        for name in ('../evil', '/absolute', 'a/../../evil', 'a\\evil', 'C:/evil'):
            with self.subTest(name=name), self.assertRaises(ValueError):
                module.safe_path(name)

if __name__ == '__main__':
    unittest.main()
