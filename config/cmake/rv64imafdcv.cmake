#include(CMakeForceCompiler)

# usage
# cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/rv32imac.cmake ../
find_program(
  RISCV_GCC_COMPILER
  NAMES riscv64-unknown-linux-gnu-gcc riscv64-unknown-elf-gcc riscv64-linux-gnu-gcc riscv64-elf-gcc
  HINTS "$ENV{RISCV}/bin" "/opt/riscv/bin" "/opt/homebrew/bin/")

if (RISCV_GCC_COMPILER)
  message( "RISC-V GCC found: ${RISCV_GCC_COMPILER}")
else()
  message(FATAL_ERROR "RISC-V GCC not found. ${RISCV_GCC_COMPILER}")
endif()



get_filename_component(RISCV_TOOLCHAIN_BIN_PATH ${RISCV_GCC_COMPILER} DIRECTORY)
get_filename_component(RISCV_TOOLCHAIN_BIN_GCC ${RISCV_GCC_COMPILER} NAME_WE)
get_filename_component(RISCV_TOOLCHAIN_BIN_EXT ${RISCV_GCC_COMPILER} EXT)

message( "RISC-V GCC Path: ${RISCV_TOOLCHAIN_BIN_PATH}" )
STRING(REGEX REPLACE "\-gcc" "-" CROSS_COMPILE ${RISCV_TOOLCHAIN_BIN_GCC})
message( "RISC-V Cross Compile: ${CROSS_COMPILE}" )

# The Generic system name is used for embedded targets (targets without OS) in CMake
set( CMAKE_SYSTEM_NAME          Generic )
set( CMAKE_SYSTEM_PROCESSOR     rv64imafdcv )
set( CMAKE_EXECUTABLE_SUFFIX    ".elf")

set(CMAKE_ASM_COMPILER ${RISCV_TOOLCHAIN_BIN_PATH}/${CROSS_COMPILE}gcc )
set(CMAKE_AR ${RISCV_TOOLCHAIN_BIN_PATH}/${CROSS_COMPILE}ar)
set(CMAKE_ASM_COMPILER ${RISCV_TOOLCHAIN_BIN_PATH}/${CROSS_COMPILE}gcc)
set(CMAKE_C_COMPILER ${RISCV_TOOLCHAIN_BIN_PATH}/${CROSS_COMPILE}gcc)
set(CMAKE_CXX_COMPILER ${RISCV_TOOLCHAIN_BIN_PATH}/${CROSS_COMPILE}g++)

# We must set the OBJCOPY setting into cache so that it's available to the
# whole project. Otherwise, this does not get set into the CACHE and therefore
# the build doesn't know what the OBJCOPY filepath is
set( CMAKE_OBJCOPY      ${RISCV_TOOLCHAIN_BIN_PATH}/${CROSS_COMPILE}objcopy
     CACHE FILEPATH "The toolchain objcopy command " FORCE )

set( CMAKE_OBJDUMP      ${RISCV_TOOLCHAIN_BIN_PATH}/${CROSS_COMPILE}objdump
     CACHE FILEPATH "The toolchain objdump command " FORCE )

# Set the common build flags

# Set the CMAKE C flags (which should also be used by the assembler!
set(CMAKE_C_FLAGS "-g -march=${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "" )
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "" )
set(CMAKE_LINKER_FLAGS   "${CMAKE_LINKER_FLAGS}  -march=${CMAKE_SYSTEM_PROCESSOR} -nostartfiles" )
