cmake_minimum_required(VERSION 3.24)
project(riscv_freestanding LANGUAGES C ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie -fno-asynchronous-unwind-tables")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie -fno-asynchronous-unwind-tables")

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--strip-all -Wl,--build-id=none")

add_executable(basic_a "${CMAKE_CURRENT_LIST_DIR}/basic_a.c" "${CMAKE_CURRENT_LIST_DIR}/start.S")
set_target_properties(basic_a PROPERTIES PREFIX "" SUFFIX ".elf")
target_link_options(basic_a PRIVATE -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
#target_link_options(basic_a PRIVATE -Wl,-e,_start)
#target_link_options(basic_a PRIVATE  -Wl,-Map,$<TARGET_FILE_DIR:basic_a>/basic_a.map)

add_executable(basic_b "${CMAKE_CURRENT_LIST_DIR}/basic_b.c")
set_target_properties(basic_b PROPERTIES PREFIX "" SUFFIX ".elf")

add_executable(basic_c "${CMAKE_CURRENT_LIST_DIR}/basic_c.c")
set_target_properties(basic_c PROPERTIES PREFIX "" SUFFIX ".elf")

#add_library(basic_d STATIC "${CMAKE_CURRENT_LIST_DIR}/basic_d.c")

add_executable(basic_ebreak "${CMAKE_CURRENT_LIST_DIR}/basic_ebreak.c" "${CMAKE_CURRENT_LIST_DIR}/start.S")
set_target_properties(basic_ebreak PROPERTIES PREFIX "" SUFFIX ".elf")
target_link_options(basic_ebreak PRIVATE -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/link.ld)

add_executable(basic_fib "${CMAKE_CURRENT_LIST_DIR}/basic_fib.c" "${CMAKE_CURRENT_LIST_DIR}/start.S")
set_target_properties(basic_fib PROPERTIES PREFIX "" SUFFIX ".elf")
target_link_options(basic_fib PRIVATE -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/link.ld)

#add_executable(basic_scall_write "${CMAKE_CURRENT_LIST_DIR}/basic_scall_write.c")
#set_target_properties(basic_scall_write PROPERTIES PREFIX "" SUFFIX ".elf")


install(
  TARGETS
  basic_a basic_b basic_c
  basic_ebreak basic_fib #basic_scall_write
  ARCHIVE DESTINATION .
  RUNTIME DESTINATION .)
