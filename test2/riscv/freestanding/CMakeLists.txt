cmake_minimum_required(VERSION 3.24)
project(riscv_freestanding LANGUAGES C CXX ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie -fno-asynchronous-unwind-tables")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie -fno-asynchronous-unwind-tables")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Os -g0 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-pic -fno-pie")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--strip-debug -Wl,--discard-all -Wl,--build-id=none")

function(add_baremetal_executable tgt_name tgt_sources)
  add_executable(${tgt_name} ${tgt_sources})
  set_target_properties(${tgt_name}  PROPERTIES PREFIX "" SUFFIX ".elf")
  install(
    TARGETS ${tgt_name}
    ARCHIVE DESTINATION .
    RUNTIME DESTINATION .)
endfunction()

function(add_semihosted_executable tgt_name tgt_sources)
  add_executable(${tgt_name} ${tgt_sources} "${CMAKE_CURRENT_LIST_DIR}/start.S")
  set_target_properties(${tgt_name}  PROPERTIES PREFIX "" SUFFIX ".elf")
  target_link_options(${tgt_name} PRIVATE -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
  #target_link_options(basic_a PRIVATE -Wl,-e,_start)
  #target_link_options(basic_a PRIVATE  -Wl,-Map,$<TARGET_FILE_DIR:basic_a>/basic_a.map)
  install(
    TARGETS ${tgt_name}
    ARCHIVE DESTINATION .
    RUNTIME DESTINATION .)
endfunction()

add_baremetal_executable(basic_b "${CMAKE_CURRENT_LIST_DIR}/basic_b.c")
add_baremetal_executable(basic_c "${CMAKE_CURRENT_LIST_DIR}/basic_c.c")

add_semihosted_executable(basic_a "${CMAKE_CURRENT_LIST_DIR}/basic_a.c")
add_semihosted_executable(basic_scall "${CMAKE_CURRENT_LIST_DIR}/basic_scall.c")
add_semihosted_executable(basic_ebreak "${CMAKE_CURRENT_LIST_DIR}/basic_ebreak.c")
add_semihosted_executable(basic_fib "${CMAKE_CURRENT_LIST_DIR}/basic_fib.c")

add_semihosted_executable(checksum "${CMAKE_CURRENT_LIST_DIR}/checksum.cpp")

add_semihosted_executable(vmcall "${CMAKE_CURRENT_LIST_DIR}/vmcall.c")
add_semihosted_executable(vmcall_array "${CMAKE_CURRENT_LIST_DIR}/vmcall_array.c")
add_semihosted_executable(vmcall_enum "${CMAKE_CURRENT_LIST_DIR}/vmcall_enum.c")
add_semihosted_executable(vmcall_fork "${CMAKE_CURRENT_LIST_DIR}/vmcall_fork.c")
add_semihosted_executable(vmcall_preempt "${CMAKE_CURRENT_LIST_DIR}/vmcall_preempt.c")
add_semihosted_executable(vmcall_return "${CMAKE_CURRENT_LIST_DIR}/vmcall_return.c")
add_semihosted_executable(vmcall_stop "${CMAKE_CURRENT_LIST_DIR}/vmcall_stop.c")
