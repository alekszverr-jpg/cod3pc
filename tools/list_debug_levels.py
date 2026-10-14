"""Inventory original SP images and native DLL availability for level debugging."""
import argparse
import json
import re
import tomllib
from pathlib import Path

from check_campaign import check_campaign


def list_debug_levels(game_root, manifest_path, build_root):
    manifest = tomllib.loads(manifest_path.read_text(encoding='utf-8'))
    project = manifest.get('project', {}).get('name', 'cod3pc')
    declarations = {(manifest_path.parent / m['file_path']).resolve(): m
                    for m in manifest.get('modules', [])}
    levels = []
    for row in check_campaign(game_root, manifest_path)['campaign_modules']:
        source = game_root / row['path']
        level = source.parent.name
        if not re.fullmatch(r'[a-z][a-z0-9_]*', level):
            raise ValueError(f'unsafe debug level identifier: {level!r}')
        declaration = declarations.get(source.resolve())
        debug_map = declaration.get('debug_map') if declaration else None
        if debug_map is not None and (not isinstance(debug_map, str) or
                                      not re.fullmatch(r'[a-z][a-z0-9_]*', debug_map)):
            raise ValueError(f'unsafe debug map command for {level}')
        dll = build_root / f'{project}_{source.stem}.dll'
        native = False
        if declaration and dll.is_file():
            with dll.open('rb') as stream:
                native = stream.read(2) == b'MZ'
        levels.append(dict(level=level, map=debug_map, kind='credits' if level == 'credits' else 'level',
                           declared=row['declared'], native=native,
                           ready=bool(row['guest_path_matches'] and native and debug_map)))
    if len({row['level'] for row in levels}) != len(levels):
        raise ValueError('ambiguous duplicate debug level identifiers')
    return levels


def select_debug_level(levels, name):
    for row in levels:
        if row['level'] == name:
            if not row['ready']:
                raise ValueError(f'{name}: native DLL is not declared, built, or correctly mapped')
            return row
    raise ValueError(f'unknown debug level: {name}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-root', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--build-root', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(list_debug_levels(args.game_root, args.manifest, args.build_root)))
