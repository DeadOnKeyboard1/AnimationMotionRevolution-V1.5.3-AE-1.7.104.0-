"""Create a mod-manager release from a fresh build. Python 3 standard library."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile
from verify_pdb import dll_identity, pdb_identity

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, default=root / 'out/release')
args = parser.parse_args()
dll = args.build / 'AnimationMotionRevolution.dll'
pdb = args.build / 'AnimationMotionRevolution.pdb'
assert dll_identity(dll) == pdb_identity(pdb), 'DLL/PDB identity mismatch'
files = {'SKSE/Plugins/' + dll.name: dll.read_bytes(),
         'SKSE/Plugins/' + pdb.name: pdb.read_bytes(),
         'readmes/GPL-3.0-or-later.txt': (root / 'LICENSE').read_bytes(),
         'readmes/AMR-MIT-LICENSE.txt': (root / 'LICENSES/MIT-AnimationMotionRevolution.txt').read_bytes(),
         'readmes/NOTICE.txt': (root / 'NOTICE.md').read_bytes(),
         'readmes/CommonLibSSE-LICENSE.txt': (root / 'external/CommonLibSSE/LICENSE').read_bytes()}
for copyright in (args.build / 'vcpkg_installed/x64-windows-skse/share').glob('*/copyright'):
    files['readmes/licenses/' + copyright.parent.name + '.txt'] = copyright.read_bytes()
dist = root / 'dist'
dist.mkdir(exist_ok=True)
output = dist / 'AnimationMotionRevolution-1.5.4-GitHubBuild-PDB.zip'
with zipfile.ZipFile(output, 'x', zipfile.ZIP_DEFLATED) as z:
    for name, data in files.items(): z.writestr(name, data)
with zipfile.ZipFile(output) as z:
    assert z.testzip() is None
    assert all(z.read(name) == data for name, data in files.items())
manifest = {name: hashlib.sha256(data).hexdigest() for name, data in files.items()}
output.with_suffix('.manifest.json').write_text(json.dumps(manifest, indent=2))
print(output)
