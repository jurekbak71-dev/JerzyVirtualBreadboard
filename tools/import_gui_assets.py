"""Import only the pinned v0.4 prototype pack; never substitute production approval."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import zipfile

LOCK = Path(__file__).with_name('gui-assets-v04.lock.json')

def safe_path(name):
    path = PurePosixPath(name)
    if path.is_absolute() or '..' in path.parts or '\\' in name or ':' in name:
        raise ValueError(f'Unsafe archive path: {name}')
    return path

def import_assets(archive, target):
    lock = json.loads(LOCK.read_text())
    if hashlib.sha256(Path(archive).read_bytes()).hexdigest() != lock['archive_sha256']:
        raise ValueError('Archive SHA256 differs from supplied v0.4 pack')
    contents = {}
    with zipfile.ZipFile(archive) as z:
        names = [i.filename for i in z.infolist() if not i.is_dir()]
        if len(names) != len(set(names)) or set(names) != set(lock['files']):
            raise ValueError('Archive inventory mismatch')
        for name in names:
            safe_path(name)
            data = z.read(name)
            if hashlib.sha256(data).hexdigest() != lock['files'][name]['sha256']:
                raise ValueError(f'File SHA256 mismatch: {name}')
            contents[name] = data
    target = Path(target)
    # Validate everything, including existing output, before any write.
    for name, data in contents.items():
        dest = target / name
        if dest.is_symlink() or any(p.is_symlink() for p in [target, *dest.parents]):
            raise ValueError('Symlinks are not accepted in import destination')
        if dest.exists() and dest.read_bytes() != data:
            raise ValueError(f'Refusing to overwrite changed file: {dest}')
    for name, data in contents.items():
        dest = target / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        if not dest.exists():
            dest.write_bytes(data)
    print(f'Validated/imported {len(contents)} files from prototype v{lock["version"]}')

def verify_assets(target):
    lock = json.loads(LOCK.read_text())
    for name, metadata in lock['files'].items():
        data = (Path(target) / safe_path(name)).read_bytes()
        if hashlib.sha256(data).hexdigest() != metadata['sha256']:
            raise ValueError(f'File SHA256 mismatch: {name}')
    print(f"Verified {len(lock['files'])} prototype files against v0.4 lock")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path, nargs='?')
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--target', type=Path, default=Path(__file__).resolve().parents[1] / 'assets/gui/production')
    args = parser.parse_args()
    if args.verify:
        verify_assets(args.target)
    elif args.archive:
        import_assets(args.archive, args.target)
    else:
        parser.error('supply archive or --verify')
