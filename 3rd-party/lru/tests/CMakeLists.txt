# ##############################################################################
# SOURCES
# ##############################################################################

set(TEST_LRU_CACHE_SOURCES
    move-awareness-test.cpp
    last-accessed-test.cpp
    iterator-test.cpp
    cache-test.cpp
    statistics-test.cpp
    wrap-test.cpp
    callback-test.cpp)

# ##############################################################################
# TARGET
# ##############################################################################

add_executable(lru-cache-test ${TEST_LRU_CACHE_SOURCES})
target_link_libraries(lru-cache-test PRIVATE catch-main lru-cache)

add_test(
  NAME lru-cache-test
  COMMAND lru-cache-test
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
