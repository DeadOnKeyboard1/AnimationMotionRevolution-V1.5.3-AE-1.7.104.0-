# Dependency provenance

- **CommonLibSSE-NG `v12.1.0`**
  - Upstream: https://github.com/alandtse/CommonLibSSE-NG
  - Commit: `8ffc2b74af015f837de53cc02b0cb470a15678e4` (`v12.1.0`)
  - License: `GPL-3.0-or-later`
  - Configured by `CMakeLists.txt` in flatrim mode (`ENABLE_SKYRIM_SE=ON`,
    `ENABLE_SKYRIM_AE=ON`, `ENABLE_SKYRIM_VR=OFF`, `SKSE_SUPPORT_XBYAK=ON`,
    `COMMONLIB_ENABLE_IPO=OFF`, `COMMONLIB_PREBUILT=OFF`).
  - Standalone builds clone and check out this pinned commit automatically;
    local/offline builds may supply the verified commit via
    `SKSE_COMMONLIB_SOURCE_ROOT`.
  - Build-time adaptation: `CMakeLists.txt` applies
    `cmake/ModuleDiscovery.inl` onto `REL/Module.h` in the binary overlay
    directory to preserve AMR's loaded-module discovery while retaining NG's
    main-module fallback.
- **MinHook HDE64 (`v1.3.4`)**
  - Upstream: https://github.com/TsudaKageyu/minhook
  - Tag: `v1.3.4` (`src/hde/hde64.c`, static helper `amr_instruction_decoder`)
  - Used for instruction-boundary validation in `include/utils/HookValidation.h`
    (and by CommonLibSSE-NG's patch-safety checks), fetched via CMake
    `FetchContent` (`SOURCE_SUBDIR src/hde`) or supplied via `AMR_HDE_ROOT`.
- **vcpkg dependencies (`vcpkg.json`)**
  - Resolved through the Microsoft vcpkg registry baseline
    `9e593bb18ea69cc5095e012465dcd675a822ed0d`, pinned in
    `vcpkg-configuration.json`:
    `boost-atomic`, `boost-stl-interfaces`, `rsm-binary-io`, `span-lite`,
    `xbyak`, `tsl-ordered-map`, `fmt`, `spdlog`, `rapidcsv`, `directxtk`.
  - The `cmake/x64-windows-skse.cmake` triplet uses static third-party
    libraries and the dynamic Microsoft CRT (`/MD`). Runtime VC++
    redistributables are not bundled.
- **Historical reference snapshot (`external/CommonLibSSE`)**
  - The trimmed `CommonLibSSE` `3.6.0` snapshot (`CharmedBaryon/CommonLibSSE`,
    commit `c4ab853d095e81e3390b282d7ba01ab2f24ebf25`, MIT) is kept under
    `external/CommonLibSSE` (along with its `LICENSE` and historical
    `include/REL/Relocation.h`) for historical reference and license notice
    retention. It is no longer compiled into `AnimationMotionRevolution`.

## Licensing and authorship

- Downstream Skyrim SE/AE compatibility work and combined binaries are
  distributed under `GPL-3.0-or-later` (`LICENSE`, `NOTICE.md`) while retaining
  every applicable MIT notice.
- Original AMR copyright remains with Alejandro / alexsylex, under MIT
  (`LICENSES/MIT-AnimationMotionRevolution.txt`).
- This is a downstream compatibility build, not a claim of upstream authorship.
