cmake_minimum_required(VERSION 3.24)
project(riscv_samples LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie")

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--strip-all -Wl,--build-id=none")


add_executable(basic_a "${CMAKE_CURRENT_LIST_DIR}/basic_a.c")
set_target_properties(basic_a PROPERTIES PREFIX "" SUFFIX ".elf")

#add_library(basic_d STATIC "${CMAKE_CURRENT_LIST_DIR}/basic_d.c")

add_executable(basic_ebreak "${CMAKE_CURRENT_LIST_DIR}/basic_ebreak.c")
set_target_properties(basic_ebreak PROPERTIES PREFIX "" SUFFIX ".elf")

add_executable(basic_fib "${CMAKE_CURRENT_LIST_DIR}/basic_fib.c")
set_target_properties(basic_fib PROPERTIES PREFIX "" SUFFIX ".elf")

#add_executable(basic_scall_write "${CMAKE_CURRENT_LIST_DIR}/basic_scall_write.c")
#set_target_properties(basic_scall_write PROPERTIES PREFIX "" SUFFIX ".elf")

install(
  TARGETS
    basic_a
    basic_ebreak basic_fib #basic_scall_write
  ARCHIVE DESTINATION .
  RUNTIME DESTINATION .)
