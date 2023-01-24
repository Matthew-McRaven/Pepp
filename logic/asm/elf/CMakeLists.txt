cmake_minimum_required(VERSION 3.10)


project(elf VERSION 3.0.0)

if (NOT "${BOOST_ROOT} " MATCHES "^[ \t\r\n]*$")
    # Do nothing, var is already set.
elseif (NOT "$ENV{BOOST_ROOT} " MATCHES "^[ \t\r\n]*$")
    set(BOOST_ROOT $ENV{BOOST_ROOT})
else ()
    # Directory in which to look for a boost install. Allowed to be empty, since there may be a system boost.
    message("Not attempting to set BOOST_ROOT")
endif ()

include(./.cmake/create_targets.cmake)

# Needed at top level so that we report the right standard to the console.
inject_cxx_standard()

find_package(Boost 1.75.0 REQUIRED)
add_subdirectory(3rd-party)


make_napi_module(bind-elf bind 0)
target_link_libraries(bind-elf PUBLIC elfio fmt magic_enum ${CMAKE_JS_LIB})