if(RISCV_GCC_FREESTANDING_COMPILER)
  ExternalProject_Add(
    riscv_freestanding
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/freestanding"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/freestanding"
    INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/freestanding"
    CMAKE_ARGS
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/config/cmake/rv64imafdc_freestanding.cmake
      -DCMAKE_BUILD_TYPE=MinSizeRel
      -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_LIST_DIR}/freestanding)
endif()

qt6_add_executable(riscv_elf_samples
                   "${CMAKE_CURRENT_LIST_DIR}/riscv_elf_samples.cpp")
qt6_add_resources(
  riscv_elf_samples
  "freestanding"
  PREFIX
  "/freestanding"
  BASE
  "${CMAKE_CURRENT_LIST_DIR}/freestanding"
  FILES
  freestanding/basic_a.elf
  freestanding/basic_b.elf
  freestanding/basic_c.elf
  freestanding/basic_ebreak.elf
  freestanding/basic_fib.elf
  freestanding/basic_scall.elf
  freestanding/brutal_fib50.elf
  freestanding/checksum.elf
  freestanding/custom_instr.elf
  freestanding/fp_pi.elf
  freestanding/fp_verify.elf
  freestanding/memprotect.elf
  freestanding/memtrap_exec.elf
  freestanding/memtrap_rw.elf
  freestanding/va_execute.elf
  freestanding/vmcall.elf
  freestanding/vmcall_array.elf
  freestanding/vmcall_fork.elf
  freestanding/vmcall_enum.elf
  freestanding/vmcall_preempt.elf
  freestanding/vmcall_return.elf
  freestanding/vmcall_stop.elf
  #
)

set_target_properties(riscv_elf_samples PROPERTIES FOLDER "qtc_runnable")
target_link_libraries(riscv_elf_samples PRIVATE pepp-lib catch Qt6::Core)
add_test(NAME riscv_elf_samples COMMAND riscv_elf_samples)
