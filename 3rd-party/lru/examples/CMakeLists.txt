###########################################################
## BINARIES
###########################################################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples)

########################################
# TARGETS
########################################

add_executable(fibonacci-basic fibonacci-basic.cpp)
target_link_libraries(fibonacci-basic lru-cache)
add_executable(statistics statistics.cpp)
target_link_libraries(statistics lru-cache)
add_executable(callbacks callbacks.cpp)
target_link_libraries(callbacks lru-cache)
add_executable(wrap wrap.cpp)
target_link_libraries(wrap lru-cache)
