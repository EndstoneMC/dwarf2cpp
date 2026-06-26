cmake_minimum_required(VERSION 3.6)

project(dwarf2cpp LANGUAGES CXX)

find_package(Python 3.12 REQUIRED COMPONENTS Interpreter Development.Module Development.SABIModule)
find_package(nanobind CONFIG REQUIRED)
find_package(LLVM CONFIG REQUIRED)

nanobind_add_module(_dwarf STABLE_ABI NB_STATIC src/dwarf2cpp/_dwarf.cpp)
target_link_libraries(_dwarf PRIVATE llvm-core::llvm-core)

install(TARGETS _dwarf DESTINATION dwarf2cpp)
