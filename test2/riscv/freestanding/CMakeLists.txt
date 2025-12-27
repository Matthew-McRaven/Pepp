cmake_minimum_required(VERSION 3.24)
project(riscv_freestanding LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie")

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--strip-all -Wl,--build-id=none")


add_executable(basic_b "${CMAKE_CURRENT_LIST_DIR}/basic_b.c")
set_target_properties(basic_b PROPERTIES PREFIX "" SUFFIX ".elf")

add_executable(basic_c "${CMAKE_CURRENT_LIST_DIR}/basic_c.c")
set_target_properties(basic_c PROPERTIES PREFIX "" SUFFIX ".elf")



install(
  TARGETS
    basic_b basic_c
  ARCHIVE DESTINATION .
  RUNTIME DESTINATION .)
