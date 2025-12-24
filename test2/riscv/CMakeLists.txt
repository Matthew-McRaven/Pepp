ExternalProject_Add(
  riscv_samples
  SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/riscv_samples"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/riscv_samples"
  INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/riscv_samples"
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/config/cmake/rv64imafdcv.cmake
    -DCMAKE_BUILD_TYPE=MinSizeRel
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_LIST_DIR}/riscv_samples
  # LOG_CONFIGURE 1 LOG_BUILD 1
)

qt6_add_executable(basic "${CMAKE_CURRENT_LIST_DIR}/basic.cpp")
qt6_add_resources(
  basic
  "riscv_samples"
  PREFIX
  "/riscv_samples"
  BASE
  "${CMAKE_CURRENT_LIST_DIR}/riscv_samples"
  FILES
  riscv_samples/basic_a.elf
  riscv_samples/basic_b.elf
  riscv_samples/basic_c.elf
  riscv_samples/basic_ebreak.elf
  riscv_samples/basic_fib.elf
  # riscv_samples/basic_scall_write.elf
)
set_target_properties(basic PROPERTIES FOLDER "qtc_runnable")

target_link_libraries(basic PRIVATE pepp-lib catch Qt6::Core)
