# Build verification

The GitHub preparation is validated locally on Windows x64 with Visual Studio
2026 (MSVC 19.51), the pinned vcpkg baseline and the `release` preset.

The validation uses a new `out/release` directory and a newly restored
`out/release/vcpkg_installed` dependency tree. It does not link the previous
AMR DLL, objects or installed CommonLib binary. CommonLib is compiled from
`external/CommonLibSSE` and the AMR translation units use the same REL header.

The release packaging script checks the DLL/PDB CodeView GUID and age, then
reopens the ZIP and checks every archived file. The source exporter verifies
all exported files against SHA-256 and rejects binaries or personal paths.

This is build and packaging verification, not a byte-for-byte reconstruction
of the old pathfix release and not an in-game regression test. GitHub-hosted
CI must still run after uploading; no remote run or repository publication
was performed during local preparation.
