# Animation Motion Revolution - Skyrim 1.7.104 Compatibility Source

Downstream source for the AMR 1.5.4 pathfix update. Original mod by
[alexsylex](https://github.com/alexsylex/AnimationMotionRevolution),
[Nexus page](https://www.nexusmods.com/skyrimspecialedition/mods/50258).
The original AMR source is MIT-licensed and its author attribution is retained.
The Skyrim 1.7.104 compatibility work uses GPL-3.0-or-later CommonLibSSE-NG
material, so this downstream combined source and its binaries are distributed
under GPL-3.0-or-later.

## Scope

This branch retains the supplied Skyrim Steam 1.7.104 compatibility changes,
Address Library format-5 support and module-relative Address Library lookup.
Use the matching SKSE 2.3.1 and Address Library for that runtime.
SE/AE code paths remain in the source; this branch does not claim tests on
other runtimes. VR is not built by the release preset.

## Build

Requirements: Windows x64, Visual Studio C++ desktop tools with C++23 support
(tested locally with Visual Studio 2026), Windows SDK, CMake, Ninja, Git, and
vcpkg. Python 3 is needed only for packaging and verification.

Bootstrap a vcpkg checkout at the pinned baseline:

```powershell
git clone https://github.com/microsoft/vcpkg
git -C vcpkg checkout 9e593bb18ea69cc5095e012465dcd675a822ed0d
.\vcpkg\bootstrap-vcpkg.bat -disableMetrics
$env:VCPKG_ROOT = (Resolve-Path .\vcpkg).Path
```

Keep that checkout outside this repository, then from this repository run:

```powershell
.\scripts\Build.ps1 -VcpkgRoot $env:VCPKG_ROOT
python .\scripts\verify_pdb.py .\out\release\AnimationMotionRevolution.dll .\out\release\AnimationMotionRevolution.pdb
python .\scripts\Package.py
```

Alternatively, in an x64 Visual Studio developer terminal with `VCPKG_ROOT` set:

```powershell
cmake --preset release
cmake --build --preset release
```

CommonLib is compiled from this repository, not taken from an installed binary.
vcpkg restores the remaining pinned dependencies into `out/release`.
No game installation, game executable, Address Library or sibling source folder
is required to compile. Building does not deploy into Skyrim.

## Install / Publish

`scripts/Package.py` creates a mod-manager ZIP containing DLL, matching PDB and
all required licenses under `dist/`. Upload that ZIP to a GitHub Release, not to the source
tree. Install it as a replacement for AMR, not as a second active AMR DLL.
The script refuses to overwrite an existing release ZIP.

Keep `build/`, `out/`, `dist/`, DLLs, PDBs, private CMake presets, game files and
Address Library databases out of Git. `.gitignore` covers generated artifacts.
No submodules are required. See [DEPENDENCIES.md](DEPENDENCIES.md) for provenance
and [RELEASE-NOTES.md](RELEASE-NOTES.md) for the distinction from the old binary.

## Validation limits

A successful build/PDB check is not a gameplay test. Test animation-driven
movement and startup through your actual Vortex/MO2 profile before publishing
the new rebuild as stable. Retain the previous pathfix release for comparison.

## License and copyright

The downstream compatibility work is Copyright (C) 2026 DeadOnKeyboard and is
licensed under GNU GPL version 3 or, at your option, any later version. See
[LICENSE](LICENSE) and [NOTICE.md](NOTICE.md).

Original Animation Motion Revolution portions are Copyright (c) 2022 Alejandro
and remain available under the MIT license. The complete original notice is
preserved in
[LICENSES/MIT-AnimationMotionRevolution.txt](LICENSES/MIT-AnimationMotionRevolution.txt).
