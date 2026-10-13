"""Offline, read-only XEX2 headers and resource inventory; never decrypts images."""
import argparse
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def version(word):
    return f'{word >> 28}.{(word >> 24) & 15}.{(word >> 8) & 65535}.{word & 255}'


def inspect_xex(path):
    data = path.read_bytes()
    def unpack(fmt, offset):
        if offset < 0 or offset + struct.calcsize(fmt) > len(data):
            raise ValueError(f'truncated XEX header at {offset:#x}')
        return struct.unpack_from(fmt, data, offset)
    if data[:4] != b'XEX2':
        raise ValueError('expected XEX2 magic')
    flags, image_offset, reserved, security, count = unpack('>5I', 4)
    if count > (len(data) - 24) // 8:
        raise ValueError('optional header table exceeds file')
    headers = dict(unpack('>II', 24 + i * 8) for i in range(count))
    report = {'file': path.name, 'sha256': sha256(path), 'size': len(data),
              'module_flags': hex(flags), 'image_offset': hex(image_offset),
              'optional_headers': {hex(k): hex(v) for k, v in headers.items()}}
    for key, label in [(0x10100, 'entry_point'), (0x10201, 'image_base')]:
        if key in headers:
            report[label] = hex(headers[key])
    if 0x40006 in headers:
        media, ver, base, title, platform, table, disc, discs, save = unpack('>4I4BI', headers[0x40006])
        report.update(title_id=f'{title:08X}', media_id=f'{media:08X}',
                      version=version(ver), base_version=version(base),
                      version_word=hex(ver), platform=platform,
                      disc_number=disc, disc_count=discs,
                      title_update='not independently established')
    if 0x3ff in headers:
        size, encryption, compression = unpack('>IHH', headers[0x3ff])
        report.update(encryption=encryption, compression=compression)
    report.update(image_size=unpack('>I', security + 4)[0],
                  load_address=hex(unpack('>I', security + 0x110)[0]),
                  region_mask=hex(unpack('>I', security + 0x178)[0]))
    libraries = []
    if 0x103ff in headers:
        start = headers[0x103ff]
        total, string_size, string_count = unpack('>III', start)
        if total < 12 or start + total > len(data) or string_size > total - 12:
            raise ValueError('invalid import string table bounds')
        # Strings are NUL terminated and each starts at a four-byte boundary.
        names, pos = [], start + 12
        while len(names) < string_count and pos < start + 12 + string_size:
            end = data.find(b'\0', pos, start + 12 + string_size)
            if end < 0:
                raise ValueError('unterminated import name')
            names.append(data[pos:end].decode('ascii'))
            pos = (end + 4) & ~3
        pos = start + 12 + string_size
        while pos < start + total:
            size = unpack('>I', pos)[0]
            idx, entries = unpack('>HH', pos + 0x24)
            if size < 0x28 + entries * 4 or pos + size > start + total or idx >= len(names):
                raise ValueError('invalid import library bounds')
            addresses = unpack(f'>{entries}I', pos + 0x28)
            libraries.append({'name': names[idx], 'record_count': entries,
                              'record_addresses': [hex(a) for a in addresses]})
            pos += size
    report['import_libraries'] = libraries
    report['import_note'] = 'Records are guest addresses, not resolved ordinals; use SDK image analysis.'
    report['sections_note'] = 'PE sections require an SDK-loaded image; not inferred from compressed payload.'
    return report


def inventory(root):
    files = []
    for path in sorted(root.rglob('*')):
        if not path.is_file() or path.name == '.gitkeep':
            continue
        with path.open('rb') as stream:
            signature = stream.read(16).hex()
        files.append({'path': path.relative_to(root).as_posix(),
                      'size': path.stat().st_size, 'prefix16_hex': signature})
    return {'file_count': len(files), 'total_bytes': sum(f['size'] for f in files),
            'extensions': dict(Counter(Path(f['path']).suffix.lower() for f in files)),
            'files': files,
            'dependency_note': 'Dependencies are not established by signatures alone.'}


def xex_paths(root, files):
    """Find genuine XEX images from the inventory, regardless of extension."""
    return [root / row['path'] for row in files
            if row['prefix16_hex'].startswith('58455832')]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_root', type=Path)
    parser.add_argument('--output', type=Path, default=Path('reports/game-analysis.json'))
    args = parser.parse_args()
    report = inventory(args.game_root)
    executable_paths = xex_paths(args.game_root, report['files'])
    report['executables'] = [dict(inspect_xex(p), path=p.relative_to(args.game_root).as_posix())
                             for p in executable_paths]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(f"{report['file_count']} files; {len(report['executables'])} XEX; report: {args.output}")


if __name__ == '__main__':
    main()
