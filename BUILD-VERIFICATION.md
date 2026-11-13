# Build verification

Version **1.5.4.5** is verified on Windows x64 both via the standalone GitHub
Actions workflow (`Windows build`, run `43514660994`, `windows-2025`,
Visual Studio 2026 / MSVC 19.51, pinned `vcpkg` baseline
`9e593bb18ea69cc5095e012465dcd675a822ed0d`, `CMakePresets.json` `release`
preset) and via the local Release/Phase-1 build (`AMR_PHASE1_BUILD=ON`,
`AMR_MOTION_DIAGNOSTICS=OFF`).

- **CommonLibSSE-NG & dependencies**: Compiled against pinned
  `alandtse/CommonLibSSE-NG` `v12.1.0`
  (`8ffc2b74af015f837de53cc02b0cb470a15678e4`, SE/AE flat-rim) with the
  `cmake/ModuleDiscovery.inl` overlay for `REL/Module.h`, `MinHook` `v1.3.4`
  (`src/hde/hde64.c`), and the pinned `vcpkg` dependencies (`x64-windows-skse`).
- **Regression tests**: Local build ran `amr_regression_tests.exe`
  (`PASS: 66 regression checks`).
- **Packaging & PDB verification**: `scripts/Package.py` validates PE/CodeView
  GUID and age consistency (`scripts/verify_pdb.py`), archives DLL, PDB, INI,
  and license notices, then reopens and verifies every entry in the ZIP.
- **Privacy verification (Release `v1.5.4.5`)**: The published
  `AnimationMotionRevolution-1.5.4.5.zip` was compiled in an isolated
  workspace path with compiler `/pathmap:` (`>DoK<`), the PDB was stripped of
  private blocks with `pdbcopy -p`, and all packaged files (`DLL`, `INI`, `PDB`)
  checked with `0` local/user path leaks.

This constitutes build, static, regression, packaging, and privacy
verification; runtime gameplay behavior in Skyrim SE/AE still requires testing
in an active SKSE/mod-manager profile.
