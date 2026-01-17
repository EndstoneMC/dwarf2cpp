# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

dwarf2cpp generates C++ headers from DWARF debug information in compiled binaries. It uses pybind11 C++ bindings to access LLVM's DWARF DebugInfo module and reconstructs source headers from debug symbols.

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
clang-format -i src/dwarf2cpp/*.cpp src/dwarf2cpp/*.h
```

## Architecture

### Data Flow
```
Binary File → DWARFContext (C++ pybind11) → Visitor (Python) → Models → Jinja2 Templates → Post-process → .h files
```

### Key Modules

- **`src/dwarf2cpp/_dwarf.cpp`** - pybind11 bindings exposing LLVM DWARF types: DWARFContext, DWARFDie, DWARFUnit, DWARFTypePrinter
- **`src/dwarf2cpp/visitor.py`** - Visitor pattern traversing DWARF DIE tree; ~25 `visit_*` methods for different DWARF tags; converts to Python models
- **`src/dwarf2cpp/models.py`** - Dataclass-based AST: Namespace, Function, Struct, Class, Union, Enum, TypeDef, Template, etc.
- **`src/dwarf2cpp/filters.py`** - Jinja2 filters for namespace handling and template rendering
- **`src/dwarf2cpp/post_process.py`** - Regex-based cleanup (std::__1/std::__ndk1 aliasing, type simplification)
- **`src/dwarf2cpp/templates/`** - Jinja2 templates for C++ header generation

### Entry Points
- CLI: `dwarf2cpp.cli:main` (Click-based)
- Module: `python -m dwarf2cpp` → `__main__.py` → `cli.py`

## Build System

- **Python build**: scikit-build-core with Conan
- **C++ build**: CMake 3.15+
- **Dependencies**: LLVM 19.1.7, pybind11 3.0.1, libxml2 (via Conan)
- **Compiler settings**: C++17 required (MSVC 193 / GCC 11 / Apple Clang 13)

## Code Style

- Python: Ruff with 120 char line length, import sorting enabled
- C++: clang-format with LLVM-based style, 99 char column limit
