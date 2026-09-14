# 1.5.4 GitHub source preparation

Baseline release: `AnimationMotionRevolution-1.5.4-Skyrim-1.7.104-pathfix.zip`.
Baseline DLL SHA-256:
`52EC4E63EDCD5617DADDB597A1952A221F35F6BA65036971DF236C67081CED43`.
The original archive and original root/build DLLs were not replaced.

The existing pathfix and format-5 source changes are retained. This preparation
vendors the matching CommonLib source, consolidates the REL header, removes
the stale external submodule/registry dependency, aligns manifest versioning,
adds author metadata, and documents a fresh source build and release packaging.
No animation hooks or movement algorithms were intentionally changed.

The new DLL is a separate rebuild, not a byte-identical reproduction of the
older release. Compiler/linker differences and the shared REL definition can
change generated code. Test the new artifact in game before promoting it to
a stable release; the old pathfix ZIP remains the known baseline.

The checked-in GitHub workflow is a build workflow, not an automatic publisher.
It has not run on GitHub until you upload the repository and trigger a run.

## Licensing

This downstream compatibility build is distributed under GPL-3.0-or-later due
to its CommonLibSSE-NG compatibility material. The original AMR MIT license and
copyright notice remain included and are not replaced by this downstream
license choice.
