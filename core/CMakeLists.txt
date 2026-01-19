cmake_minimum_required(VERSION 3.10)
inject_cxx_standard()
inject_clang_tidy()
inject_dwarf_debug()
inject_code_coverage()

# Build this library statically, because it will be consumed in two different
# SOs The first is the actual pepplib library, and the other is nanobind.
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
add_library(pepp-core STATIC ${sources})
target_link_libraries(pepp-core PUBLIC fmt spdlog cxxgraph flat)
target_compile_features(pepp-core PUBLIC cxx_std_20)

# ######### Options from libriscv
if(APPLE)
  find_library(SECURITY_FRAMEWORK Security)
  find_library(FOUNDATION_FRAMEWORK Foundation)
  mark_as_advanced(SECURITY_FRAMEWORK FOUNDATION_FRAMEWORK)

  target_link_libraries(pepp-core PUBLIC "${SECURITY_FRAMEWORK}"
                                         "${FOUNDATION_FRAMEWORK}")
endif()

option(RISCV_DEBUG "Enable extra checks in the RISC-V machine" OFF)
# Enable Floating-point Control and Status Register emulation
option(RISCV_FCSR "Enable FCSR emulation" OFF)
# MEMORY_TRAPS allows you to trap writes to uncacheable pages in memory. Cached
# pages can only be trapped once.
option(RISCV_MEMORY_TRAPS "Enable memory page traps" ON)

if(NOT (WIN32 OR MINGW_TOOLCHAIN))
  target_compile_options(pepp-core PRIVATE -Wall -Wextra)
endif()
if(RISCV_MULTIPROCESS)
  find_package(Threads REQUIRED)
  target_link_libraries(pepp-core PUBLIC Threads::Threads)
endif()
if(WIN32 OR MINGW_TOOLCHAIN)
  target_link_libraries(pepp-core PUBLIC wsock32 ws2_32)
endif()
# ######### End options from libriscv
