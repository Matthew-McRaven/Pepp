set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# todo: search for clang using `which`
if(DEFINED $ENV{CMAKE_C_COMPILER})
    set(CMAKE_C_COMPILER $ENV{CMAKE_C_COMPILER})
elseif()
    set(CMAKE_C_COMPILER /usr/bin/clang)
endif()
if(DEFINED $ENV{CMAKE_CXX_COMPILER})
    set(CMAKE_CXX_COMPILER $ENV{CMAKE_CXX_COMPILER})
elseif()
    set(CMAKE_CXX_COMPILER /usr/bin/clang++)
endif()

