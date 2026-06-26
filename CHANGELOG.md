# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Changed
- Switched the Python build backend from scikit-build-core-conan to conan-py-build
- Upgraded LLVM to 22.1.7 and replaced the vendored `DWARFTypePrinter` workaround with LLVM's own
- Migrated the C++ bindings from pybind11 to nanobind, built against the stable ABI
  (`Py_LIMITED_API`) so a single `cp312-abi3` wheel covers Python 3.12+

### Removed
- Dropped Python 3.10 and 3.11 support (nanobind's stable ABI requires 3.12+)

## [0.1.0] - 2026-05-16

### Added
- Initial release
- Generate C++ headers from DWARF debug information
- Support for Windows (MSVC), Linux (GCC), and macOS (Apple Clang)
- pybind11 bindings for LLVM DWARF DebugInfo
- Jinja2 template-based header generation
