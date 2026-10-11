"""Local SVOD extraction using the pinned, reviewed rexauto reader only."""
import argparse
import importlib.util
import json
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REXAUTO_COMMIT = 'dac49b331f2632f0b050f469e35747721300b8b2'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('header', type=Path)
    parser.add_argument('--out', type=Path, default=ROOT / 'assets/game')
    parser.add_argument('--executables-only', action='store_true')
    args = parser.parse_args()
    repo = ROOT / 'external/rexauto'
    git = Path('C:/Program Files/Git/cmd/git.exe')
    commit = subprocess.check_output([str(git), '-C', str(repo), 'rev-parse', 'HEAD'], text=True).strip()
    if commit != REXAUTO_COMMIT:
        raise SystemExit('rexauto revision changed: review extraction code before updating pin')
    spec = importlib.util.spec_from_file_location('rexauto_extract', repo / 'extract.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    header = args.header.resolve()
    data = header.read_bytes()
    if data[:4] not in (b'LIVE', b'PIRS', b'CON ') or struct.unpack_from('>I', data, 0x3a9)[0] != 1:
        raise SystemExit('expected an SVOD package header')
    reader = module._svod_reader(str(header), data, print)
    files = module._walk_gdfx(reader, *struct.unpack_from('<II', reader.vd, 0x14))
    # Validate every destination before any write, and reject case collisions.
    output = args.out.resolve()
    seen = set()
    for rel, sector, size in files:
        dest = (output / rel).resolve()
        if not dest.is_relative_to(output) or ':' in rel or '\\' in rel:
            raise SystemExit(f'unsafe container path: {rel!r}')
        if rel.casefold() in seen:
            raise SystemExit(f'duplicate container path: {rel!r}')
        seen.add(rel.casefold())
    report_dir = ROOT / 'reports'
    report_dir.mkdir(exist_ok=True)
    (report_dir / 'container-files.json').write_text(json.dumps(files, indent=2), encoding='utf-8')
    if args.executables_only:
        for rel, sec, size in files:
            if not rel.lower().endswith('.xex'):
                continue
            dest = output / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            payload = reader(sec, (size + 0x7ff) & ~0x7ff)[:size]
            if len(payload) != size or payload[:4] != b'XEX2':
                raise SystemExit(f'invalid executable: {rel}')
            dest.write_bytes(payload)
    else:
        module._gdfx_extract(reader, str(output), print)
    for rel, sec, size in files:
        if args.executables_only and not rel.lower().endswith('.xex'):
            continue
        dest = output / rel
        if not dest.is_file() or dest.stat().st_size != size:
            raise SystemExit(f'extraction verification failed: {rel}')
    print('Extraction sizes verified; no package signature/hash-tree verification performed.')


if __name__ == '__main__':
    main()
