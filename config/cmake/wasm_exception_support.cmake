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

  message(STATUS "PEPP_HAS_QT_WASM_EXCEPTIONS: ${PEPP_HAS_QT_WASM_EXCEPTIONS}")
  if(PEPP_HAS_QT_WASM_EXCEPTIONS)
    add_compile_options(-fwasm-exceptions)
    add_link_options(-fwasm-exceptions)

    # Catch2 attempts to determine whether C++ exceptions are available. Under
    # -fwasm-exceptions, they do not appear to be available, so Catch
    # auto-defines CATCH_CONFIG_DISABLE_EXCEPTIONS. We know that exceptions
    # work, so we can override Catch's detection. Must be public, otherwise we
    # will have to hunt down each TU which links catch.
    if(TARGET catch)
      target_compile_definitions(catch
                                 PUBLIC CATCH_CONFIG_NO_DISABLE_EXCEPTIONS)
    endif()
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
# unquoted ${PEPP_TESTS_SKIP_THROWS_ARGS} so it vanishes when empty rather than
# adding a stray blank argument.
set(PEPP_TESTS_SKIP_THROWS_ARGS "")
if(PEPP_TESTS_SKIP_THROWS)
  # Trailing * isn't decorative. Catch's argument parser only checks --nothrow
  # if there is a positional argument. Since filters are AND'ed together, this
  # is effectively a no-op. Failure to include this * cause us to run throwing
  # tests on exceptionless platforms; misery ensues.
  list(APPEND PEPP_TESTS_SKIP_THROWS_ARGS --nothrow "*")
endif()
