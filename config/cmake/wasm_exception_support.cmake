include_guard()

# Path to the node launcher for Emscripten test binaries. Qt's wasm output is
# MODULARIZE'd , so `node <target>.js` does nothing and returns 0, making it
# appear as if the tests passed. This script loads the WASM module, patches the
# global environment to fool Qt about our headless env, and executes the tests.
set(PEPP_WASM_TEST_RUNNER "${CMAKE_CURRENT_LIST_DIR}/wasm_test_runner.js")

# Detects whether the active toolchain has enabled native wasm exception
# handling (-fwasm-exceptions). Must be places after Qt6 is found.
#
# Sets PEPP_HAS_QT_WASM_EXCEPTIONS to ON or OFF. Always defined, even when
# Emscripten or Qt6 are not in use. If -fwasm-exceptions are detected, then we
# also globally add -fwasm-exceptions to our compile/link flags so targets
# without a Qt dependency will pick it up too.
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

# Pre-built COMMAND argument list for add_test() call sites. Insert it unquoted
# ${PEPP_TESTS_SKIP_THROWS_ARGS} so it vanishes when empty rather than adding a
# stray, blank argument.
set(PEPP_TESTS_SKIP_THROWS_ARGS "")
if(PEPP_TESTS_SKIP_THROWS)
  # Trailing * is really required. Catch's argument parser only checks --nothrow
  # if there is a positional argument. Since filters are AND'ed together, this
  # is effectively a no-op. Failure to include this * cause us to run throwing
  # tests on exceptionless platforms; misery ensues.
  list(APPEND PEPP_TESTS_SKIP_THROWS_ARGS --nothrow "*")
endif()
