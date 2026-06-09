cmake_minimum_required(VERSION 3.16)

qt_add_executable(simexp main.cpp simuloop.cpp)

target_link_libraries(simexp PRIVATE pepp-core)
set_target_properties(simexp PROPERTIES FOLDER "qtc_runnable")
