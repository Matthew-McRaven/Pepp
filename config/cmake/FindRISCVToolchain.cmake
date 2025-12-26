find_program(
  RISCV_GCC_COMPILER
  NAMES riscv64-unknown-elf-gcc riscv64-linux-gnu-gcc riscv64-elf-gcc
  HINTS "$ENV{RISCV}/bin" "/opt/riscv/bin" "/opt/homebrew/bin/")

if(RISCV_GCC_COMPILER)
  set(HAVE_RISCV_TOOLCHAIN TRUE)
  message(STATUS "RISC-V toolchain found at ${RISCV_GCC_COMPILER}")
else()
  set(HAVE_RISCV_TOOLCHAIN FALSE)
  message(STATUS "RISC-V toolchain not found")
endif()
