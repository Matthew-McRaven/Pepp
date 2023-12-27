make_exec(demo-antlr "./")

file(GLOB_RECURSE antlr-sources CONFIGURE_DEPENDS "antlr/*.h" "antlr/*.cpp")
add_library(demo-antlr-generated OBJECT ${antlr-sources})
# Will fail to find hpp files without this, see https://bugreports.qt.io/browse/QTBUG-101146.
get_target_property(ANTLR4_INCLUDE_DIR antlr4_static INCLUDE_DIRECTORIES)
target_include_directories(demo-antlr-generated PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${ANTLR4_INCLUDE_DIR} ${CMAKE_CURRENT_LIST_DIR}/antlr)
target_include_directories(demo-antlr PRIVATE src)


target_link_libraries(demo-antlr-generated PUBLIC antlr4_static)
target_link_libraries(demo-antlr PRIVATE Qt6::Quick demo-antlr-generated)

