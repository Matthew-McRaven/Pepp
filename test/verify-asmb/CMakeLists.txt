file(GLOB srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# End-to-end assembly of the textbook figures.
qt6_add_executable(verify-asmb ${srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(verify-asmb PRIVATE pepp-lib catch Qt6::Core)
set_target_properties(verify-asmb PROPERTIES FOLDER "qtc_runnable")
pepp_precompile_headers(verify-asmb PRIVATE <QtCore>)

# One ctest entry per file. Use the filename stem rather than a shard count to
# provide more meaningful output in ctest. A stem matching no tests exits 2
# rather than silently passing.
if(NOT IOS)
  foreach(src IN LISTS srcs)
    get_filename_component(stem "${src}" NAME_WE)
    # WASM execution requires an extra shim script
    if(EMSCRIPTEN)
      add_test(
        NAME verify-asmb-${stem}
        COMMAND
          ${CMAKE_CROSSCOMPILING_EMULATOR} "${PEPP_WASM_TEST_RUNNER}"
          $<TARGET_FILE:verify-asmb> --filenames-as-tags "[#${stem}]"
          ${PEPP_TESTS_SKIP_THROWS_ARGS})
    else()
      add_test(NAME verify-asmb-${stem}
               COMMAND $<TARGET_FILE:verify-asmb> --filenames-as-tags
                       "[#${stem}]" ${PEPP_TESTS_SKIP_THROWS_ARGS})
    endif()
  endforeach()

  # Longest running tests in suite. Provide cost data to ctest to encourage it
  # to start those tests early.
  set_tests_properties(verify-asmb-pep10-unified PROPERTIES COST 10)
  set_tests_properties(verify-asmb-pep10-standalone PROPERTIES COST 10)
  set_tests_properties(verify-asmb-pep9-figs PROPERTIES COST 20)
  set_tests_properties(verify-asmb-pep10-figs PROPERTIES COST 20)
endif()
