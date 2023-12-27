ADD_SUBDIRECTORY(elfio)
ADD_SUBDIRECTORY(cli)
ADD_SUBDIRECTORY(textflowcpp)
add_subdirectory(catch)
# Disable unit tests for antlr, since it produces many CMake warnings.
# We don't use VCPKG, since it ships only x86 binaries for Mac,
# and we target ARM plus x86, which means we need to build from the sources.
set(ANTLR_BUILD_CPP_TESTS CACHE INTERNAL FALSE FORCE)
ADD_SUBDIRECTORY(antlr4)
