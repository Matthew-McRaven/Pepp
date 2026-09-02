file(GLOB instr_srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Exhaustive per-mnemonic ISA verification against the core simulator. No
# dependency on Qt, so it can use catch-main directly.
add_executable(verify-pep10-new ${instr_srcs})
target_link_libraries(verify-pep10-new PRIVATE pepp-core catch-main)
set_target_properties(verify-pep10-new PROPERTIES FOLDER "qtc_runnable")

# One ctest entry per source file. Use the filename stem rather than a shard
# count to provide more meaningful output in ctest. A stem matching no tests
# exits 2 rather than silently passing.
if(NOT (EMSCRIPTEN OR IOS))
  foreach(src IN LISTS instr_srcs)
    get_filename_component(stem "${src}" NAME_WE)
    add_test(NAME verify-pep10-new-${stem}
             COMMAND $<TARGET_FILE:verify-pep10-new> --filenames-as-tags
                     "[#${stem}]")
  endforeach()
endif()
