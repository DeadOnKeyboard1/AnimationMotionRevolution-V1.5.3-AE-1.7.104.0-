# Animation Motion Revolution v1.5.4.5

Universal Skyrim SE/AE compatibility release (including Skyrim AE `1.7.104.0`,
built in SE/AE flat-rim configuration against CommonLibSSE-NG `v12.1.0`).

## Highlights in 1.5.4.5

- **CommonLibSSE-NG `v12.1.0` migration** (`8ffc2b74af015f837de53cc02b0cb470a15678e4`):
  Replaces the older `3.6.0` build baseline with `CommonLibSSE-NG v12.1.0`
  (`ENABLE_SKYRIM_SE=ON`, `ENABLE_SKYRIM_AE=ON`, `ENABLE_SKYRIM_VR=OFF`) and
  a narrow `REL/Module.h` overlay (`cmake/ModuleDiscovery.inl`) preserving
  AMR's loaded-module override and discovery behavior.
- **Runtime layout & Address Library format-5 validation**: Added explicit
  runtime/offset verification (`include/RuntimeLayout.h`), Address Library
  format-5 support, and module-relative database lookup.
- **Hook safety & rollback**: Added `MinHook` `v1.3.4` HDE64 instruction-length
  validation (`include/utils/HookValidation.h`), `include/utils/CallPatchRollback.h`,
  and hardened trampoline helpers (`include/utils/Trampoline.h`).
- **Motion handling & diagnostics configuration**: Updated `CharacterClipAnimMotionMap`,
  `AnimMotionHandler`, `Hooks`, and `Settings` (`config/AnimationMotionRevolution.ini`
  with `[Debug]` `uLogLevel` and bounded optional diagnostics
  `bEnableMotionDiagnostics=0` by default).
- **Regression tests & CI**: Added unit/regression suite (`tests/Regression.cpp`,
  `66 regression checks`), standalone `git`/vcpkg GitHub Actions workflow
  (`.github/workflows/build.yml`, green on `windows-2025`), and privacy-clean
  `v1.5.4.5` release packaging (DLL, INI, public `pdbcopy -p` PDB).

## Licensing

This downstream compatibility build is distributed under `GPL-3.0-or-later`
([LICENSE](LICENSE)) due to its `CommonLibSSE-NG` compatibility material. The
original AMR MIT license and copyright notice remain preserved in
[LICENSES/MIT-AnimationMotionRevolution.txt](LICENSES/MIT-AnimationMotionRevolution.txt)
and [NOTICE.md](NOTICE.md).
