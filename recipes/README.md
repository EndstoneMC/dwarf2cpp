# Local Conan recipes

Local Conan recipes maintained for this project, not (yet) available on
[conan-center-index](https://github.com/conan-io/conan-center-index).

## `llvm-core/22.1.7`

A local build of the upstream conan-center-index `llvm-core` recipe, bumped to
**LLVM 22.1.7** (conan-center only ships up to `19.1.7`).

Differences from the upstream recipe:

- `conandata.yml` points at the LLVM 22.1.7 release. LLVM 22 **stopped publishing
  the split per-component source tarballs** (`llvm-*.src.tar.xz` /
  `cmake-*.src.tar.xz`); only the full `llvm-project-*.src.tar.xz` monorepo is
  released.
- `source()` gained a branch that fetches the monorepo and renames its `llvm/`
  subdirectory to `llvm-main/` so the rest of the recipe (which already expects
  that layout for LLVM >= 18) works unchanged. The `cmake/` module directory is
  already present as a sibling inside the monorepo.
- `patches/22x/0000-cmake-dependencies.patch` is the `19x` cmake-dependencies
  patch re-cut against the 22.1.7 sources (line offsets only; content identical).

The rest of the recipe is unmodified — all of its version gates (`>= 15`,
`>= 18`, `>= 19`) already resolve correctly for 22.x.

### Export / upload to Cloudsmith

```bash
# Export the recipe into the local Conan cache
conan export recipes/llvm-core/all --version 22.1.7

# (Optional) build it locally to confirm it works end to end
conan create recipes/llvm-core/all --version 22.1.7 --build=missing

# Add the Cloudsmith remote (once) and upload the RECIPE ONLY (no prebuilt binary).
# Consumers then build LLVM from source with --build=missing.
conan remote add cloudsmith <your-cloudsmith-conan-url>
conan upload "llvm-core/22.1.7" -r cloudsmith --only-recipe --confirm
```

This recipe also adds two options not present in the upstream conan-center recipe,
`tools` and `utils` (both default `True` to preserve upstream behaviour), mapping to
`LLVM_INCLUDE_TOOLS` / `LLVM_INCLUDE_UTILS`. With `tools=False` the Windows-only
`LLVM_BUILD_LLVM_C_DYLIB` is also disabled. `llvm-tblgen` is always built regardless,
so the LLVM build still works with both off.

Because we publish the recipe only, every consumer builds LLVM from source. The
project's `conanfile.py` trims that build via options — `targets=X86;AArch64`,
`tools=False`, `utils=False`, `with_z3=False`, `with_libedit=False` — since dwarf2cpp
only links LLVM libraries to read object files and parse DWARF (no codegen backends,
command-line tools, test utilities, SMT solver, or line editor needed). zlib/zstd are
kept so compressed debug sections can still be decoded.
