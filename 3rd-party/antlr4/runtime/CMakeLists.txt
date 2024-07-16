option(TRACE_ATN "Trace ATN simulation" OFF)
option(ANTLR_BUILD_SHARED "Build the shared library of the ANTLR runtime" ON)
option(ANTLR_BUILD_STATIC "Build the static library of the ANTLR runtime" ON)

if (NOT ANTLR_BUILD_SHARED AND NOT ANTLR_BUILD_STATIC)
    message(FATAL_ERROR "Options ANTLR_BUILD_SHARED and ANTLR_BUILD_STATIC can't both be OFF")
endif ()

include_directories(
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/src/atn
        ${CMAKE_CURRENT_LIST_DIR}/src/dfa
        ${CMAKE_CURRENT_LIST_DIR}/src/internal
        ${CMAKE_CURRENT_LIST_DIR}/src/misc
        ${CMAKE_CURRENT_LIST_DIR}/src/support
        ${CMAKE_CURRENT_LIST_DIR}/src/tree
        ${CMAKE_CURRENT_LIST_DIR}/src/tree/pattern
        ${CMAKE_CURRENT_LIST_DIR}/src/tree/xpath
)


file(GLOB libantlrcpp_SRC
        "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/atn/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/dfa/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/internal/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/misc/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/support/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/tree/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/tree/pattern/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/tree/xpath/*.cpp"
)

if (ANTLR_BUILD_SHARED)
    add_library(antlr4_shared SHARED ${libantlrcpp_SRC})
endif ()
if (ANTLR_BUILD_STATIC)
    add_library(antlr4_static STATIC ${libantlrcpp_SRC})
endif ()

if (CMAKE_HOST_UNIX)
    # Make sure to link against threads (pthreads) library in order to be able to
    # make use of std::call_once in the code without producing runtime errors
    # (see also https://github.com/antlr/antlr4/issues/3708 and/or https://stackoverflow.com/q/51584960).
    find_package(Threads REQUIRED)

    if (TARGET antlr4_shared)
        target_link_libraries(antlr4_shared Threads::Threads)
    endif ()
    if (TARGET antlr4_static)
        target_link_libraries(antlr4_static Threads::Threads)
    endif ()
endif ()

IF (TRACE_ATN)
    ADD_DEFINITIONS(-DTRACE_ATN_SIM=1)
ENDIF (TRACE_ATN)

if (APPLE)
    if (TARGET antlr4_shared)
        target_link_libraries(antlr4_shared ${COREFOUNDATION_LIBRARY})
    endif ()
    if (TARGET antlr4_static)
        target_link_libraries(antlr4_static ${COREFOUNDATION_LIBRARY})
    endif ()
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(disabled_compile_warnings "/wd4251")
else ()
    set(disabled_compile_warnings "-Wno-overloaded-virtual")
endif ()


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(disabled_compile_warnings "${disabled_compile_warnings} -Wno-dollar-in-identifier-extension -Wno-four-char-constants")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set(disabled_compile_warnings "${disabled_compile_warnings} -Wno-multichar")
endif ()

set(extra_share_compile_flags "")
set(extra_static_compile_flags "")
set(static_lib_suffix "")

if (WIN32)
    set(static_lib_suffix "-static")
    if (TARGET antlr4_shared)
        target_compile_definitions(antlr4_shared PRIVATE ANTLR4CPP_EXPORTS)
    endif ()
    if (TARGET antlr4_static)
        target_compile_definitions(antlr4_static PUBLIC ANTLR4CPP_STATIC)
    endif ()
    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(extra_share_compile_flags "-MP /wd4251")
        set(extra_static_compile_flags "-MP")
    endif ()
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

if (TARGET antlr4_shared)
    set_target_properties(antlr4_shared
            PROPERTIES COMPILE_FLAGS "${disabled_compile_warnings} ${extra_share_compile_flags}")
endif ()

if (TARGET antlr4_static)
    set_target_properties(antlr4_static
            PROPERTIES VERSION ${ANTLR_VERSION}
            COMPILE_FLAGS "${disabled_compile_warnings} ${extra_static_compile_flags}")
endif ()
