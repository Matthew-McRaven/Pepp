include_guard()
if (EXISTS "${CMAKE_SOURCE_DIR}/node_modules/@pepnext/device-interface")
    message(STATUS "Using local device-interface")
    include(${CMAKE_SOURCE_DIR}/node_modules/@pepnext/device-interface/CMakeLists.txt)
elseif (${CMAKE_SOURCE_DIR}/../@pepnext/device-interface)
    message(STATUS "Using sibling device-interface")
    include(${CMAKE_SOURCE_DIR}/../@pepnext/device-interface/CMakeLists.txt)
else ()
    message(FATAL "Couldn't find device-interface")
endif ()
