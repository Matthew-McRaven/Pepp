cmake_minimum_required(VERSION 3.10)

make_interface(symbol symbol)
target_include_directories(symbol INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(symbol INTERFACE fmt)

make_test(symbol-test test)
target_link_libraries(symbol-test PUBLIC symbol)

if (true)
    # Bad, ugly, evil
    include_directories(${CMAKE_JS_INC} ../../../node_modules/node-addon-api/ ${NODE_ROOT}/include/node)
    #message(${NODE_ROOT}/include/node)
    make_napi_module(bind-symbol16 bind 16)
    target_link_libraries(bind-symbol16 PUBLIC symbol fmt magic_enum)
    make_napi_module(bind-symbol32 bind 32)
    target_link_libraries(bind-symbol32 PUBLIC symbol fmt magic_enum)
endif ()