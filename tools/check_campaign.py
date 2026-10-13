"""Read-only campaign XEX inventory and native manifest coverage.

Declaration is not proof of successful compilation, loading or gameplay.
Original hashes and header metadata are written only to a private report.
"""
import argparse
import json
import tomllib
from pathlib import Path

from inspect_game import inspect_xex, inventory, xex_paths


def check_campaign(game_root, manifest_path):
    manifest = tomllib.loads(manifest_path.read_text(encoding='utf-8'))
    declarations = {}
    for module in manifest.get('modules', []):
        source = (manifest_path.parent / module['file_path']).resolve()
        if source in declarations:
            raise ValueError(f'duplicate module source: {source}')
        declarations[source] = module
    rows = []
    game_inventory = inventory(game_root)
    for image in xex_paths(game_root, game_inventory['files']):
        relative = image.relative_to(game_root)
        if relative.parts[0].casefold() != 'sp':
            continue
        header = inspect_xex(image)
        declaration = declarations.get(image.resolve())
        guest_path = declaration.get('guest_path') if declaration else None
        rows.append(dict(path=relative.as_posix(), sha256=header['sha256'],
                         image_base=header.get('image_base', header['load_address']),
                         entry_point=header.get('entry_point'),
                         image_size=header['image_size'], declared=declaration is not None,
                         guest_path=guest_path,
                         guest_path_matches=(guest_path.replace('\\', '/').casefold() ==
                                             relative.as_posix().casefold()) if guest_path else False))
    return dict(campaign_modules=rows, count=len(rows),
                declared_count=sum(row['declared'] for row in rows),
                note='Manifest declarations are not runtime or gameplay verification.')


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-root', type=Path, default=root / 'assets/game')
    parser.add_argument('--manifest', type=Path, default=root / 'cod3pc/cod3pc_manifest.toml')
    parser.add_argument('--output', type=Path, default=root / 'reports/campaign-readiness.json')
    args = parser.parse_args()
    report = check_campaign(args.game_root, args.manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    for row in report['campaign_modules']:
        status = 'declared' if row['declared'] else 'not recompiled in manifest'
        print(f"{row['path']}: base={row['image_base']}, entry={row['entry_point']}, {status}")
    print(f"{report['count']} genuine campaign XEX; {report['declared_count']} manifest declarations")


if __name__ == '__main__':
    main()
