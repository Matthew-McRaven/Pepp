
cmake_minimum_required(VERSION 3.10)
file(GLOB_RECURSE cs6e-res-abs CONFIGURE_DEPENDS "books/cs6e/*")

SET(cs6e-res, "")
foreach(f-abs IN LISTS cs6e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs6e-res "${f-rel}")
endforeach()
message(${cs6e-res})
include(../../../config/cmake/create_targets.cmake)

enable_testing()
make_library(builtins builtins)
target_include_directories(builtins PRIVATE fmt)
target_link_libraries(builtins PRIVATE "fmt;Qt6::Core")
qt_add_resources(builtins "builts" PREFIX "/"
        FILES ${cs6e-res})

make_test(builtins-test test "builtins;Qt6::Core")

