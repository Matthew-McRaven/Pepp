########################################
# SOURCES
########################################

set(TEST_LRU_CACHE_SOURCES
  move-awareness-test.cpp
  last-accessed-test.cpp
  iterator-test.cpp
  cache-test.cpp
  statistics-test.cpp
  wrap-test.cpp
  callback-test.cpp
  test-main.cpp
)

########################################
# TARGET
########################################

add_executable(lru-cache-test ${TEST_LRU_CACHE_SOURCES})

set_target_properties(lru-cache-test PROPERTIES FOLDER "qtc_runnable")
target_link_libraries(lru-cache-test catch)

add_test(
  NAME lru-cache-test
  COMMAND lru-cache-test
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
