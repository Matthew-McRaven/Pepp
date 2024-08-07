file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
add_pepp_test(TARGET macro TESTS ${tests} DEPENDS Qt6::Gui Qt6::Qml Qt6::Quick)
