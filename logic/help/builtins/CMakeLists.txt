cmake_minimum_required(VERSION 3.10)


file(GLOB_RECURSE cs4e-res-abs CONFIGURE_DEPENDS "books/cs4e/*")
SET(cs4e-res, "")
foreach(f-abs IN LISTS cs4e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs4e-res "${f-rel}")
endforeach()

file(GLOB_RECURSE cs5e-res-abs CONFIGURE_DEPENDS "books/cs5e/*")
SET(cs5e-res, "")
foreach(f-abs IN LISTS cs5e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs5e-res "${f-rel}")
endforeach()

file(GLOB_RECURSE cs6e-res-abs CONFIGURE_DEPENDS "books/cs6e/*")
SET(cs6e-res, "")
foreach(f-abs IN LISTS cs6e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs6e-res "${f-rel}")
endforeach()

make_library(builtins builtins)
target_link_libraries(builtins PUBLIC "Qt6::Core;Qt6::Gui;macro")
target_compile_definitions(builtins PRIVATE BUILTINS_LIBRARY)
qt_add_resources(builtins "builtins" PREFIX "/" FILES ${cs4e-res} ${cs5e-res} ${cs6e-res})

if(NOT IOS)
  make_qtest(test-builtins test/dummy.cpp "builtins;Qt6::Core;Qt6::Gui;Qt6::Qml;Qt6::Quick;Qt::Test")
endif()
