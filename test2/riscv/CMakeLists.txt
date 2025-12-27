if(RISCV_GCC_LINUX_COMPILER)
  ExternalProject_Add(
    riscv_hosted
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/hosted"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/hosted"
    INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/hosted"
    CMAKE_ARGS
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/config/cmake/rv64imafdcv_linux.cmake
      -DCMAKE_BUILD_TYPE=MinSizeRel
      -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_LIST_DIR}/hosted)
endif()
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

qt6_add_executable(basic "${CMAKE_CURRENT_LIST_DIR}/basic.cpp")
qt6_add_resources(
  basic
  "freestanding"
  PREFIX
  "/freestanding"
  BASE
  "${CMAKE_CURRENT_LIST_DIR}/freestanding"
  FILES
  freestanding/basic_b.elf
  freestanding/basic_c.elf)
qt6_add_resources(
  basic
  "hosted"
  PREFIX
  "/hosted"
  BASE
  "${CMAKE_CURRENT_LIST_DIR}/hosted"
  FILES
  hosted/basic_a.elf
  hosted/basic_ebreak.elf
  hosted/basic_fib.elf
  # riscv_samples/basic_scall_write.elf
)

set_target_properties(basic PROPERTIES FOLDER "qtc_runnable")
target_link_libraries(basic PRIVATE pepp-lib catch Qt6::Core)
add_test(NAME basic COMMAND basic)
