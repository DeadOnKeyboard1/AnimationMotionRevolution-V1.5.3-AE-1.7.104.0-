"""Export only repository sources, never local build/deployment files."""
import hashlib
import json
import re
from pathlib import Path
import zipfile

root = Path(__file__).resolve().parents[1]
directories = ('.github', 'cmake', 'include', 'source', 'external', 'scripts')
root_files = ('.clang-format', '.gitignore', 'CMakeLists.txt', 'CMakePresets.json',
              'vcpkg.json', 'vcpkg-configuration.json', 'LICENSE', 'README.md',
              'DEPENDENCIES.md', 'RELEASE-NOTES.md', 'BUILD-VERIFICATION.md', 'version.rc.in')
files = [root / name for name in root_files]
for name in directories:
    files.extend(p for p in (root / name).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
for path in files:
    if path.is_symlink() or path.suffix.lower() in {'.dll','.pdb','.lib','.obj','.exe','.zip','.bin'}:
        raise ValueError(f'Unexpected non-source file: {path}')
    if re.search(rb'[A-Za-z]:[\\/]+Users[\\/]', path.read_bytes()):
        raise ValueError(f'Personal path in source: {path}')
names = {p.relative_to(root).as_posix(): p for p in files}
assert len(names) == len(files)
manifest = {name: hashlib.sha256(p.read_bytes()).hexdigest() for name,p in sorted(names.items())}
dist = root / 'dist'
dist.mkdir(exist_ok=True)
output = dist / 'AnimationMotionRevolution-1.5.4-GitHub-Source.zip'
with zipfile.ZipFile(output, 'x', zipfile.ZIP_DEFLATED) as z:
    for name,p in sorted(names.items()):
        z.writestr(name, p.read_bytes())
with zipfile.ZipFile(output) as z:
    assert z.testzip() is None
    assert set(z.namelist()) == set(manifest)
    for name,digest in manifest.items():
        assert hashlib.sha256(z.read(name)).hexdigest() == digest
output.with_suffix('.manifest.json').write_text(json.dumps(manifest, indent=2))
print(f'{output}\nVerified {len(files)} source files; no binaries, caches or personal paths.')
