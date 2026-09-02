file(GLOB instr_srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Exhaustive per-mnemonic ISA verification.
qt6_add_executable(verify-pep9-isa3 ${instr_srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(verify-pep9-isa3 PRIVATE pepp-lib catch Qt6::Core)
set_target_properties(verify-pep9-isa3 PROPERTIES FOLDER "qtc_runnable")
pepp_precompile_headers(verify-pep9-isa3 PRIVATE <QtCore>)

# One ctest entry per mnemonic. Use the filename stem rather than a shard count
# to provide more meaningful output in ctest. A stem matching no tests exits 2
# rather than silently passing.
if(NOT IOS)
  foreach(src IN LISTS instr_srcs)
    get_filename_component(stem "${src}" NAME_WE)
    # WASM execution requires an extra shim script
    if(EMSCRIPTEN)
      add_test(
        NAME verify-pep9-isa3-${stem}
        COMMAND
          ${CMAKE_CROSSCOMPILING_EMULATOR} "${PEPP_WASM_TEST_RUNNER}"
          $<TARGET_FILE:verify-pep9-isa3> --filenames-as-tags "[#${stem}]"
          ${PEPP_TESTS_SKIP_THROWS_ARGS})
    else()
      add_test(NAME verify-pep9-isa3-${stem}
               COMMAND $<TARGET_FILE:verify-pep9-isa3> --filenames-as-tags
                       "[#${stem}]" ${PEPP_TESTS_SKIP_THROWS_ARGS})
    endif()
  endforeach()
endif()
