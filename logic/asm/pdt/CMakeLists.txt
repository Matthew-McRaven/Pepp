cmake_minimum_required(VERSION 3.10)


project(logict-pdt VERSION 3.0.0)

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
#add_subdirectory(3rd-party)

#enable_testing()
#make_library(logic-pdt pdt)
#set_property(TARGET logic-pdt PROPERTY POSITION_INDEPENDENT_CODE ON)
#target_include_directories(logic-pdt PUBLIC ${Boost_INCLUDE_DIRS})
#target_link_libraries(logic-pdt PUBLIC magic_enum)

#make_test(logic-pdt-test test/pdt logic-pdt)

#make_napi_module(bind-targets-pep10-16 bind 16)
#target_link_libraries(bind-targets-pep10-16 PUBLIC targets-pep10-isa magic_enum ${CMAKE_JS_LIB})
#make_napi_module(bind-targets-pep10-32 bind 32)
#target_link_libraries(bind-targets-pep10-32 PUBLIC targets-pep10-isa magic_enum ${CMAKE_JS_LIB})