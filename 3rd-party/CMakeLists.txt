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
