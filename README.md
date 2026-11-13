# Animation Motion Revolution - Universal SE/AE Compatibility Source (v1.5.4.5)

Downstream source for the **AMR 1.5.4.5** Skyrim SE/AE compatibility update
(including **Skyrim AE 1.7.104.0** / SKSE 2.3.1). Original mod by
[alexsylex](https://github.com/alexsylex/AnimationMotionRevolution),
[Nexus page](https://www.nexusmods.com/skyrimspecialedition/mods/50258).
The original AMR source is MIT-licensed and its author attribution is retained.
The downstream compatibility work uses GPL-3.0-or-later CommonLibSSE-NG
material, so this downstream combined source and its binaries are distributed
under GPL-3.0-or-later.

## Scope

This branch targets the Universal Skyrim SE/AE flat-rim baseline (`1.5.97.0`,
`1.6.1170.0`, and mandatory primary target `1.7.104.0`) with Address Library
format-5 support, module-relative Address Library lookup, runtime layout
verification (`include/RuntimeLayout.h`), and HDE64 hook/rollback safety checks.
Use the matching SKSE build and Address Library for your Skyrim runtime
(e.g. SKSE 2.3.1 for Skyrim 1.7.104.0). VR is not built by the release preset.

## Build

### Version 1.5.4.5

AMR 1.5.4.5 builds against CommonLibSSE-NG 12.1.0 (pinned commit
`8ffc2b74af015f837de53cc02b0cb470a15678e4`, SE/AE flat-rim), adds module-relative
runtime validation and safe hook/trampoline rollback, and ships
`config/AnimationMotionRevolution.ini` with `[Debug]` logging / optional bounded
motion diagnostics (`bEnableMotionDiagnostics=0` by default).

### Standalone Build

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

CommonLibSSE-NG (`v12.1.0`, `8ffc2b74af015f837de53cc02b0cb470a15678e4`) and
MinHook HDE64 (`v1.3.4`) are fetched automatically at configure time in
standalone builds (or read from local caches via `SKSE_COMMONLIB_SOURCE_ROOT`
and `AMR_HDE_ROOT`), and compiled alongside AMR rather than linked against a
prebuilt binary.
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
