file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

qt6_add_executable(pepp-term ${sources})
target_link_libraries(pepp-term PRIVATE cli Qt6::Core catch test-lib-all
                                        pepp-lib catch)
set_target_properties(pepp-term PROPERTIES FOLDER "qtc_runnable")

if(WIN32)
  install(
    TARGETS pepp-term
    COMPONENT Pepp
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .)
elseif(APPLE)
  set_target_properties(pepp-term PROPERTIES INSTALL_RPATH
                                             "@executable_path/../Frameworks")
  install(
    TARGETS pepp-term
    COMPONENT Pepp
    BUNDLE DESTINATION pepp.app/Contents/MacOS
    RUNTIME DESTINATION pepp.app/Contents/MacOS)
endif()

option(ENABLE_TERM_TESTS "Enable shell tests of pepp" ON)
if(ENABLE_TERM_TESTS AND NOT (EMSCRIPTEN OR IOS))
  # Needed to find config file.
  qt6_add_executable(
    test-term
    ../test/about.cpp
    ../test/asmrun.cpp
    ../test/ls.cpp
    ../test/get.cpp
    ../test/test_main.cpp
    ../test/samples.qrc
    ../test/samples.cpp)
  target_include_directories(test-term PRIVATE ${CMAKE_CURRENT_LIST_DIR}/test)
  target_link_libraries(test-term PRIVATE cli catch Qt6::Core)
  add_dependencies(test-term pepp-lib pepp-libplugin pepp)
  add_test(NAME test-term COMMAND test-term)
endif()

# Set pepp/pepp-term as a test target.
if(NOT (EMSCRIPTEN OR IOS))
  set(count 2)
  catch_test_count(count)
  catch_add_sharded_tests(pepp-term ADDTL_ARGS "selftest" SHARD_COUNT ${count})
endif()
