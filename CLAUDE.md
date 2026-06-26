# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

dwarf2cpp generates C++ headers from DWARF debug information in compiled binaries. It uses nanobind C++ bindings to access LLVM's DWARF DebugInfo module and reconstructs source headers from debug symbols.

## Build Commands

```bash
# Install from source (requires C++ toolchain: MSVC on Windows, GCC on Linux, Apple Clang on macOS)
pip install .

# Run the tool
python -m dwarf2cpp <binary> --base-dir <compilation-base-dir>

# Format Python code
ruff format src/

# Lint Python code
ruff check src/

# Format C++ code
clang-format -i src/dwarf2cpp/_dwarf.cpp
```

## Architecture

### Data Flow
```
Binary File → DWARFContext (C++ nanobind) → Visitor (Python) → Models → Jinja2 Templates → Post-process → .h files
```

### Key Modules

- **`src/dwarf2cpp/_dwarf.cpp`** - nanobind bindings exposing LLVM DWARF types: DWARFContext, DWARFDie, DWARFUnit, DWARFTypePrinter (built against the stable ABI / `Py_LIMITED_API` for a single cp312-abi3 wheel)
- **`src/dwarf2cpp/visitor.py`** - Visitor pattern traversing DWARF DIE tree; ~25 `visit_*` methods for different DWARF tags; converts to Python models
- **`src/dwarf2cpp/models.py`** - Dataclass-based AST: Namespace, Function, Struct, Class, Union, Enum, TypeDef, Template, etc.
- **`src/dwarf2cpp/filters.py`** - Jinja2 filters for namespace handling and template rendering
- **`src/dwarf2cpp/post_process.py`** - Regex-based cleanup (std::__1/std::__ndk1 aliasing, type simplification)
- **`src/dwarf2cpp/templates/`** - Jinja2 templates for C++ header generation

### Entry Points
- CLI: `dwarf2cpp.cli:main` (Click-based)
- Module: `python -m dwarf2cpp` → `__main__.py` → `cli.py`

## Build System

- **Python build**: conan-py-build (PEP 517 backend driving Conan); recipe in `conanfile.py`
- **C++ build**: CMake 3.26+ (Development.SABIModule for the stable ABI)
- **Dependencies**: LLVM 22.1.7, nanobind 2.12.0, libxml2 (via Conan)
- **Compiler settings**: C++17 required; Python 3.12+ (nanobind stable ABI)

## Code Style

- Python: Ruff with 120 char line length, import sorting enabled
- C++: clang-format with LLVM-based style, 99 char column limit
