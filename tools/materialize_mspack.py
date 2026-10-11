"""Materialize Git symlink placeholders in libmspack for Windows checkouts."""
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1] / 'external/rexglue-sdk/thirdparty/libmspack'
git = 'C:/Program Files/Git/cmd/git.exe'
entries = subprocess.check_output([git, '-C', str(root), 'ls-files', '-s', '-z'])
count = 0
for entry in entries.split(b'\0'):
    if not entry:
        continue
    meta, name = entry.decode().split('\t', 1)
    if not meta.startswith('120000 '):
        continue
    path = root / name
    if path.is_symlink():
        continue
    link = subprocess.check_output([git, '-C', str(root), 'show', f'HEAD:{name}']).decode().strip()
    if path.read_bytes().strip() != link.encode():
        continue  # Already materialized; do not overwrite other local edits.
    target = (path.parent / link).resolve()
    if not target.is_relative_to(root.resolve()) or not target.is_file():
        raise SystemExit(f'unsafe or missing symlink target: {name}')
    path.write_bytes(target.read_bytes())
    count += 1
print(f'Materialized {count} libmspack symlink placeholders; upstream content unchanged.')
