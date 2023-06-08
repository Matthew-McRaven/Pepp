make_exec(pepp-term "./")

# GLOB_RECURSE ueses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
set_target_properties(pepp-term PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
        )
set_target_properties(pepp-term PROPERTIES FOLDER "qtc_runnable")

# See: https://github.com/BoostGSoC14/boost.http/pull/47/commits/21db2a954ee5e6b6e3fbc98b487047158073c33a
# Must link against extra libs on windows for boot asio to work.
list(APPEND SOCK_LIB "")
if (MSVC OR MINGW)
    list(APPEND SOCK_LIB ws2_32)
    list(APPEND SOCK_LIB mswsock)
endif ()

target_link_libraries(pepp-term
        PRIVATE TextFlow cli about macro pas builtins isa targets sim Qt6::Core ${SOCK_LIB})

install(TARGETS pepp-term
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

find_package(Python3)
if (Python3_FOUND)
    add_test(NAME test-term-about COMMAND ${Python3_EXECUTABLE} test/about.py $<TARGET_FILE:pepp-term> WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    add_test(NAME test-term-ls COMMAND ${Python3_EXECUTABLE} test/ls.py $<TARGET_FILE:pepp-term> WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    add_test(NAME test-term-get COMMAND ${Python3_EXECUTABLE} test/get.py $<TARGET_FILE:pepp-term> WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    add_test(NAME test-term-asmrun COMMAND ${Python3_EXECUTABLE} test/asmrun.py $<TARGET_FILE:pepp-term> WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    add_subdirectory(test/samples)
endif ()

