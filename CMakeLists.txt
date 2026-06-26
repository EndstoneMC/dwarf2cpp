cmake_minimum_required(VERSION 3.15...4.0)

project(dwarf2cpp LANGUAGES CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)
find_package(LLVM CONFIG REQUIRED)

Python3_add_library(_dwarf MODULE
        src/dwarf2cpp/_dwarf.cpp
        WITH_SOABI)
target_link_libraries(_dwarf PRIVATE pybind11::headers llvm-core::llvm-core)
if (MSVC)
    target_compile_options(_dwarf PRIVATE /wd4244)
endif ()

install(TARGETS _dwarf DESTINATION dwarf2cpp)
