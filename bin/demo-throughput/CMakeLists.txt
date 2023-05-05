cmake_minimum_required(VERSION 3.16)

make_exec(demo-throughput "./")

# GLOB_RECURSE ueses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
set_target_properties(demo-throughput PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
set_target_properties(demo-throughput PROPERTIES FOLDER "qtc_runnable")

target_link_libraries(demo-throughput
    PRIVATE isa sim targets Qt6::Core)

install(TARGETS demo-throughput
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
