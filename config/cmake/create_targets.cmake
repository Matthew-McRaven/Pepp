include_guard()
# Allows us to not use __declspec(dll...) everywhere
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# We always want universal builds, so do not set on a per-target basis NOTE:
# This prevents us from building statically!!

# test-lib-all-int will include the sources for all catch tests, and be
# dependent on all the tests' libaries. Our test browser must depend on this
# target.
qt6_add_library(test-lib-all-int INTERFACE)
# test-all bundles all the tests into a single executable to prove that there
# are no linker errors. do not add to ctest, otherwise every test runs twice.
qt6_add_executable(test-all ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
# Make test-lib-all a library of all of our sources to reduce the number
# of object members we "bubble up" to other targets. Otherwise we can hit LNK1189 on Windows
qt6_add_library(test-lib-all)
target_link_libraries(test-lib-all PUBLIC test-lib-all-int)
set_target_properties(test-lib-all PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(test-all PROPERTIES FOLDER "qtc_runnable")
target_link_libraries(test-all PUBLIC test-lib-all catch)
# Failure to copy DLLs on Windows causes tests to fail at runtime.
if(WIN32)
  add_custom_command(
    TARGET test-all
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} ARGS -E copy $<TARGET_RUNTIME_DLLS:test-all>
            $<TARGET_FILE_DIR:test-all>
    COMMAND_EXPAND_LISTS)
endif()

# Track all libraries that have been linked to in make_target. At the end of all
# build steps we can copy all of the libraries exactly once.
define_property(GLOBAL PROPERTY ALL_LIBRARIES)
set_property(GLOBAL PROPERTY ALL_LIBRARIES "")

function(catch_test_count count_name)
  get_target_property(FILES test-lib-all INTERFACE_SOURCES)
  list(LENGTH FILES count)
  # message("Test count: ${count} with files ${FILES}")
  set(${count_name}
      ${count}
      PARENT_SCOPE)
endfunction()

function(maybe_append_all_libraries library)
  # message("Called with ${library}") Check if the custom property
  # (LINKED_LIBRARIES) exists for the target.
  get_property(
    is_defined GLOBAL
    PROPERTY ALL_LIBRARIES
    DEFINED)
  if(is_defined)
    # Property exists, check if the library is already in the list.
    get_property(linked_libraries GLOBAL PROPERTY ALL_LIBRARIES)
    # message("So far: ${linked_libraries}")
    if(NOT "${library}" IN_LIST linked_libraries)
      # message("So far ${linked_libraries}") Library not found in the list,
      # need to link it.
      target_link_libraries(test-lib-all-int INTERFACE ${library})
      # Update the LINKED_LIBRARIES property.
      list(APPEND linked_libraries ${library})
      set_property(GLOBAL PROPERTY ALL_LIBRARIES ${linked_libraries})
    endif()
  else()
    # Property does not exist, so link the library and create the property.
    target_link_libraries(test-lib-all-int INTERFACE ${library})
    set_property(GLOBAL PROPERTY ALL_LIBRARIES ${library})
    # message("It is ${is_defined}")
  endif()
endfunction()
