
cmake_minimum_required(VERSION 3.10)
file(GLOB_RECURSE cs6e-res-abs CONFIGURE_DEPENDS "books/cs6e/*")

SET(cs6e-res, "")
foreach(f-abs IN LISTS cs6e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs6e-res "${f-rel}")
endforeach()

include(../../../config/cmake/create_targets.cmake)

enable_testing()
make_library(builtins builtins)
target_link_libraries(builtins PRIVATE "Qt6::Core;Qt6::Gui;macro")
qt_add_resources(builtins "builtins" PREFIX "/" FILES ${cs6e-res})

if(NOT IOS)
  make_qtest(test-builtins test/dummy.test.cpp "builtins;Qt6::Core;Qt6::Gui;Qt6::Qml;Qt6::Quick;Qt::Test")
endif()
