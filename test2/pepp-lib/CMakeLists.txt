file(GLOB_RECURSE srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Not sharded, because individual tests are cheap. Process start time would
# dominate runtime.
qt6_add_executable(test-pepplib ${srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(test-pepplib PRIVATE pepp-lib catch Qt6::Core)
set_target_properties(test-pepplib PROPERTIES FOLDER "qtc_runnable")
pepp_precompile_headers(test-pepplib PRIVATE <QtCore>)

if(NOT (EMSCRIPTEN OR IOS))
  add_test(NAME test-pepplib COMMAND test-pepplib)
  set_tests_properties(test-pepplib PROPERTIES COST 9)
endif()
