from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class Dwarf2cppConan(ConanFile):
    name = "dwarf2cpp"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    default_options = {
        "llvm-core/*:targets": "X86;AArch64",
        "llvm-core/*:tools": False,
        "llvm-core/*:utils": False,
        "llvm-core/*:with_z3": False,
        "llvm-core/*:with_libedit": False,
    }

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("llvm-core/22.1.7")
        self.requires("libxml2/[>=2.13 <2.14]")
        self.requires("nanobind/2.12.0")

    def build_requirements(self):
        # >=3.27 for FindPython's Development.SABIModule (nanobind STABLE_ABI).
        self.tool_requires("cmake/[>=3.27]")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
