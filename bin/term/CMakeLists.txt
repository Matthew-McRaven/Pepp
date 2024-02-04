file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/test/*.hpp")
file(GLOB_RECURSE compile_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/compile-test/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})
list(REMOVE_ITEM sources ${compile_tests})

make_target(
  TARGET pepp-term
  TYPE "EXEC"
  SOURCES ${sources}
  DEPENDS TextFlow cli about macro pas builtins isa targets sim Qt6::Core
)


# GLOB_RECURSE ueses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
set_target_properties(pepp-term PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

# Non-DLL platforms have empty generator expressin, so use more generator magic to only copy DLLs if the exist.
add_custom_command(TARGET pepp-term POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
        -E $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:pepp-term>>,copy,true> $<TARGET_RUNTIME_DLLS:pepp-term> $<TARGET_FILE_DIR:pepp-term>
        COMMAND_EXPAND_LISTS
)


install(TARGETS pepp-term
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set_target_properties(pepp-term PROPERTIES FOLDER "qtc_runnable")

set_property(SOURCE "test/impl.cpp.in" PROPERTY SKIP_AUTOMOC ON)

# Stop searching for python on first match.
cmake_policy(SET CMP0094 NEW)
# Prevent registry from pre-empting our python installation, needed in CI to find debug libs.
set(Python3_FIND_REGISTRY LAST)

find_package(Python 3.12 OPTIONAL_COMPONENTS Development Interpreter)

set(CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GENERATE OUTPUT ${CONFIG_DIR}/test_config.txt CONTENT "$<TARGET_FILE:pepp-term>\n" TARGET pepp-term)
configure_file(test/config.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/config.hpp")

make_target(
  TARGET test-term
  TYPE "TEST"
  SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/test/ls.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/get.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_main.cpp
  DEPENDS Qt6::Core catch
)
# Needed to find config file.
target_include_directories(test-term PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(pepp-term test-term)
set_target_properties(test-term PROPERTIES FOLDER "qtc_runnable")
