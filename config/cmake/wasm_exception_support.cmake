include_guard()

# Path to the node launcher for Emscripten test binaries. Qt's wasm output is
# MODULARIZE'd , so `node <target>.js` does nothing and returns 0. This script
# loads the WASM module, patches the global environment to fool Qt, and executes
# the tests.
set(PEPP_WASM_TEST_RUNNER "${CMAKE_CURRENT_LIST_DIR}/wasm_test_runner.js")

# Detects whether the active toolchain has enabled native wasm exception
# handling (-fwasm-exceptions). Must be places after Qt6 is found.
#
# Sets PEPP_HAS_QT_WASM_EXCEPTIONS to ON or OFF. Always defined, even off
# Emscripten or when Qt6 is not in use. If WASM_EXCEPTIONS are detected, then we
# also globally add -fwasm-exceptions to our compile flags so non-Qt targets
# will pick it up.
set(PEPP_HAS_QT_WASM_EXCEPTIONS OFF)
if(EMSCRIPTEN AND TARGET Qt6::Platform)
  get_target_property(_pepp_qt_platform_copts Qt6::Platform
                      INTERFACE_COMPILE_OPTIONS)
  get_target_property(_pepp_qt_platform_lopts Qt6::Platform
                      INTERFACE_LINK_OPTIONS)
  if("${_pepp_qt_platform_copts}" MATCHES "-fwasm-exceptions"
     OR "${_pepp_qt_platform_lopts}" MATCHES "-fwasm-exceptions")
    set(PEPP_HAS_QT_WASM_EXCEPTIONS ON)
  endif()
  unset(_pepp_qt_platform_copts)
  unset(_pepp_qt_platform_lopts)

  # Belt and suspenders: also catch a build where the flag was added some other
  # way (manually on the command line, a different Qt version that does not
  # route through Qt6::Platform, etc). Harmless if Qt6::Platform already found
  # it.
  foreach(_flags_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_INIT
                     CMAKE_EXE_LINKER_FLAGS CMAKE_EXE_LINKER_FLAGS_INIT)
    if(${_flags_var} MATCHES "-fwasm-exceptions")
      set(PEPP_HAS_QT_WASM_EXCEPTIONS ON)
    endif()
  endforeach()

  message(STATUS "PEPP_HAS_QT_WASM_EXCEPTIONS: ${PEPP_HAS_QT_WASM_EXCEPTIONS}")
  if(PEPP_HAS_QT_WASM_EXCEPTIONS)
    add_compile_options(-fwasm-exceptions)
    add_link_options(-fwasm-exceptions)
  endif()
endif()

# Skip catch tests which throw in ctest. Normal platforms are fine with
# exceptions. WASM is the odd-one out. Unless built with exception support, we
# must avoid throwing or tests will fail unexpectedly.
set(PEPP_TESTS_SKIP_THROWS OFF)
if(EMSCRIPTEN)
  if(NOT PEPP_HAS_QT_WASM_EXCEPTIONS)
    set(PEPP_TESTS_SKIP_THROWS ON)
  endif()
endif()

# Pre-built COMMAND argument list for add_test() call sites. Splice this in
# unquoted (${PEPP_TESTS_SKIP_THROWS_ARGS}, no quotes) so it vanishes entirely
# when empty rather than adding a stray blank argument.
set(PEPP_TESTS_SKIP_THROWS_ARGS "")
if(PEPP_TESTS_SKIP_THROWS)
  list(APPEND PEPP_TESTS_SKIP_THROWS_ARGS --nothrow)
endif()
