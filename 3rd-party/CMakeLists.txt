add_subdirectory(catch)
ADD_SUBDIRECTORY(elfio)
ADD_SUBDIRECTORY(cli)
# Disable unit tests for antlr, since it produces many CMake warnings.
# We don't use VCPKG, since it ships only x86 binaries for Mac,
# and we target ARM plus x86, which means we need to build from the sources.
set(ANTLR_BUILD_CPP_TESTS CACHE INTERNAL FALSE FORCE)
# On Windows, we ship with MSVC redistributable.
# Mismatch on static CRT linkage causes compile errors on Windows.
set(WITH_STATIC_CRT CACHE INTERNAL FALSE FORCE)
ADD_SUBDIRECTORY(antlr4)

# Can be removed when https://github.com/eyalz800/zpp_bits/pull/130 is merged.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(zpp_bits INTERFACE zpp_bits/zpp_bits.h)
target_include_directories(zpp_bits INTERFACE zpp_bits)

add_subdirectory(lru)
add_subdirectory(scintilla)

set(KDDockWidgets_FRONTENDS "qtquick" CACHE STRING "" FORCE)
set(KDDW_FRONTEND_QTQUICK ON CACHE BOOL "" FORCE)
if(EMSCRIPTEN)
    set(KDDockWidgets_STATIC TRUE CACHE BOOL "" FORCE)
endif()

add_subdirectory(KDDockWidgets/)

# Inject --bind flag into KDDockWidgets targets on WASM builds
if(EMSCRIPTEN)
    get_directory_property(submodule_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/KDDockWidgets BUILDSYSTEM_TARGETS)
    foreach(tgt IN LISTS submodule_targets)
        get_target_property(existing_flags ${tgt} LINK_FLAGS)
        if(NOT existing_flags)
            set(existing_flags "")
        endif()
        set_target_properties(${tgt} PROPERTIES LINK_FLAGS "${existing_flags} --bind")
    endforeach()
endif()


