cmake_minimum_required(VERSION 3.10)


project(symbol VERSION 3.0.0)

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
make_interface(symbol symbol)
target_include_directories(symbol INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(symbol INTERFACE fmt)

make_test(symbol-test test symbol)

#message(${NODE_ROOT}/include/node)
make_napi_module(bind-symbol16 bind 16)
target_link_libraries(bind-symbol16 PUBLIC symbol fmt magic_enum ${CMAKE_JS_LIB})
make_napi_module(bind-symbol32 bind 32)
target_link_libraries(bind-symbol32 PUBLIC symbol fmt magic_enum ${CMAKE_JS_LIB})