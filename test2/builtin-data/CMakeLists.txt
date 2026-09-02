file(GLOB srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Uses Qt's resources system for access to the test data.
qt6_add_executable(test-builtins ${srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(test-builtins PRIVATE pepp-lib catch Qt6::Core)
set_target_properties(test-builtins PROPERTIES FOLDER "qtc_runnable")

# A handful of cheap integration tests -- one ctest entry, no sharding.
if(NOT (EMSCRIPTEN OR IOS))
  add_test(NAME test-builtins COMMAND test-builtins)
endif()
