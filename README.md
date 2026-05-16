# dwarf2cpp

[![Build Status](https://github.com/EndstoneMC/dwarf2cpp/actions/workflows/build.yml/badge.svg)](https://github.com/EndstoneMC/dwarf2cpp/actions)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Python Version](https://img.shields.io/badge/python-3.10%2B-blue.svg)

Generate C++ headers from DWARF Debugging Information Format (DWARF).

> [!WARNING]
> This tool requires binaries that contain **DWARF debug information** in order to generate headers.
> Make sure your input binary includes DWARF debug info, which is often not publicly available in proprietary software.

## Installation

The easiest way to get started is the prebuilt package. No C++ toolchain or LLVM required.

```
uvx dwarf2cpp
```

or with pip
```
pip install dwarf2cpp
```

### Build from Source

Building from source is only needed when no prebuilt wheel is available for your platform.
Since dwarf2cpp uses **pybind11** to access LLVM's DWARF DebugInfo module from Python, it requires a working C++ toolchain:

* On Windows: **MSVC (Visual Studio Build Tools)**
* On Linux: **GCC (g++)**
* On macOS: **Apple Clang (Xcode Command Line Tools)**

```
git clone https://github.com/EndstoneMC/dwarf2cpp.git
cd dwarf2cpp
pip install .
```

## Usage

```
Usage: dwarf2cpp [OPTIONS] PATH

Options:
  --base-dir TEXT         Base directory used during compilation.  [required]
  -o, --output-path PATH  Output directory for generated files. Defaults to
                          'out' inside the input file's directory.
  --help                  Show this message and exit.
```

The `PATH` argument must point to a binary containing DWARF debug information.

* `--base-dir` should point to the root directory used during compilation. This helps resolve relative include paths when reconstructing headers.
* `--output-path` controls where the generated headers are stored. If not specified, the tool creates an `out/` folder next to the input file.

## Examples

### Extract from `libminecraftpe.so`

```
dwarf2cpp path/to/libminecraftpe.so --base-dir D:/a/_work/1/s
```

### Extract from `bedrock_server` (Linux)

```
dwarf2cpp path/to/bedrock_server --base-dir /mnt/vss/_work/1/s
```

## Motivation / Purpose

Typical use cases include:

* Analysing the internals of proprietary software.
* Supporting the development of plugin frameworks such as **[Endstone](https://github.com/EndstoneMC/endstone)**.
* Research on automated source reconstruction from DWARF.

## Limitations

* Generated headers may not compile out-of-the-box. Manual adjustments may be required.
* Templates, inline functions, and macros cannot always be reconstructed accurately.
* Only works with binaries compiled with DWARF debug info. Stripped binaries will not work.
* Only trivially tested. It may fail with certain binaries.

## Acknowledgements

This project makes use of the following open-source technologies:

* [LLVM Project](https://llvm.org/) - DWARF DebugInfo parser.
* [pybind11](https://pybind11.readthedocs.io/) - C++/Python bindings.
* [click](https://click.palletsprojects.com/) - Command-line interface framework.

## Security / Legal Disclaimer

> [!IMPORTANT]
> This tool is provided for research and educational purposes only.
> It is **not affiliated with Mojang or Microsoft**.

> [!WARNING]
> Do not redistribute or publish headers generated from proprietary binaries without proper rights.
> Respect the terms of service and licensing agreements of any binaries you analyse.

## Releasing

1. Add changes under `## [Unreleased]` in `CHANGELOG.md`
2. Bump `version` in `pyproject.toml`, then commit and push to `main`
3. Go to **Actions > Release > Run workflow**

Use **dry run** to preview without making changes.

## Contributing

Contributions are welcome. Open an issue or pull request on GitHub.

## License

This project is distributed under the MIT License.
See the [LICENSE](LICENSE) file for more details.
