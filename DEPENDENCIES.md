# Dependency provenance

- CommonLibSSE NG 3.6.0 is vendored under `external/CommonLibSSE`.
  Upstream: https://github.com/CharmedBaryon/CommonLibSSE
  Commit: `c4ab853d095e81e3390b282d7ba01ab2f24ebf25`.
  The snapshot comes from the vcpkg source tree used for the previous build.
  Its MIT license is retained. Only build-relevant source/header/CMake files,
  the upstream README and license are included; upstream test assets are omitted.
  CommonLib's upstream tests must remain disabled for this trimmed snapshot.
- Local adaptations: `include/REL/Relocation.h` contains AMR's existing
  format-5 header/table decoder and module-relative database filename handling.
  The CMake plugin helper retains the existing post-629 compatibility flag.
  Its stale literal include-directory entry is removed; target usage
  requirements provide the include paths. Xbyak support is enabled explicitly.
  AMR's `include/REL/Relocation.h` forwards to this single canonical copy, so
  the plugin and compiled CommonLib no longer use different REL definitions.
- All other libraries resolve through the Microsoft vcpkg registry at
  `9e593bb18ea69cc5095e012465dcd675a822ed0d`, pinned in
  `vcpkg-configuration.json`. No private registry or neighboring project is needed.
- The `x64-windows-skse` triplet retains static third-party libraries and the
  dynamic Microsoft CRT. Runtime VC++ redistributables are not bundled.

This is a downstream compatibility build, not a claim of upstream authorship.
Original AMR copyright remains with Alejandro / alexsylex, under MIT.
