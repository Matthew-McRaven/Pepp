file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})

make_library(
        TARGET helpers
        SOURCES ${sources}
        DEPENDS Qt6::Core bits elfio symbol macro isa obj parse pas builtins
        TESTS ${tests}
        TEST_DEPENDS builtins targets catch catch-main
)
