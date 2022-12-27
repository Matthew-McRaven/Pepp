cmake_minimum_required(VERSION 3.23)


project(PLACEHOLDER VERSION 3.0.0)
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

# HACK, until we provide a better mechanism for building boost.
find_package(Boost 1.75.0 REQUIRED)
add_subdirectory(3rd-party)

enable_testing()
make_library(PLACEHOLDER dummy)
target_include_directories(PLACEHOLDER PUBLIC ${Boost_INCLUDE_DIRS})

make_test(PLACEHOLDER-test test PLACEHOLDER)

#message(${NODE_ROOT}/include/node)
make_napi_module(bind-PLACEHOLDER bind 0)
target_link_libraries(bind-PLACEHOLDER PUBLIC PLACEHOLDER ${CMAKE_JS_LIB})
