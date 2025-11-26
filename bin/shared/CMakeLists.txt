file(GLOB_RECURSE sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

qt6_add_library(pepp-cli ${sources})
target_link_libraries(pepp-cli PRIVATE cli Qt6::Core catch test-lib-all pepp-lib catch)
