file(GLOB_RECURSE cs4e-res-abs CONFIGURE_DEPENDS "books/cs4e/*")
SET(cs4e-res, "")
foreach (f-abs IN LISTS cs4e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs4e-res "${f-rel}")
endforeach ()

file(GLOB_RECURSE cs5e-res-abs CONFIGURE_DEPENDS "books/cs5e/*")
SET(cs5e-res, "")
foreach (f-abs IN LISTS cs5e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs5e-res "${f-rel}")
endforeach ()

file(GLOB_RECURSE cs6e-res-abs CONFIGURE_DEPENDS "books/cs6e/*")
SET(cs6e-res, "")
foreach (f-abs IN LISTS cs6e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs6e-res "${f-rel}")
endforeach ()

file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/test/*.hpp")
file(GLOB_RECURSE help_items CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/books/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})
list(REMOVE_ITEM sources ${help_items})

make_library2(
  TARGET builtins
  SOURCES ${sources}
  DEPENDS Qt6::Core macro
  TESTS ${tests}
  TEST_DEPENDS Qt6::Gui Qt6::Qml Qt6::Quick
)

qt_add_resources(builtins "builtins" PREFIX "/" FILES ${cs4e-res} ${cs5e-res} ${cs6e-res})
