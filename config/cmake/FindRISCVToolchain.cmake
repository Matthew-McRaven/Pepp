find_program(
  RISCV_GCC_LINUX_COMPILER
  NAMES riscv64-linux-gnu-gcc riscv64-unknown-linux-gnu-gcc
  HINTS "$ENV{RISCV}/bin" "/opt/riscv/bin" "/opt/homebrew/bin/")
find_program(
  RISCV_GCC_FREESTANDING_COMPILER
  NAMES riscv64-unknown-elf-gcc riscv64-elf-gcc
  HINTS "$ENV{RISCV}/bin" "/opt/riscv/bin" "/opt/homebrew/bin/")
