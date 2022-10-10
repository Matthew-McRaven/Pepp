cmake_minimum_required(VERSION 3.10)

make_library(pep10)
target_link_libraries(pep10 PUBLIC fmt utils outcome magic_enum ${Boost_LIBRARIES})
target_link_libraries(pep10 PUBLIC api symbol ir asmdr macro builtins machine pepx)

add_subdirectory(test)
