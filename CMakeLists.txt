cmake_minimum_required(VERSION 3.6)

project(dwarf2cpp LANGUAGES CXX)

find_package(Python 3.12 REQUIRED COMPONENTS Interpreter Development.Module Development.SABIModule)
find_package(nanobind CONFIG REQUIRED)
find_package(LLVM CONFIG REQUIRED)

nanobind_add_module(_dwarf STABLE_ABI NB_STATIC src/dwarf2cpp/_dwarf.cpp)
target_link_libraries(_dwarf PRIVATE llvm-core::llvm-core)

# Generate the type stub (and a PEP 561 marker) from the compiled module.
nanobind_add_stub(
        _dwarf_stub
        MODULE _dwarf
        OUTPUT _dwarf.pyi
        MARKER_FILE py.typed
        PYTHON_PATH $<TARGET_FILE_DIR:_dwarf>
        DEPENDS _dwarf
)

install(TARGETS _dwarf DESTINATION dwarf2cpp)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/_dwarf.pyi ${CMAKE_CURRENT_BINARY_DIR}/py.typed
        DESTINATION dwarf2cpp)
