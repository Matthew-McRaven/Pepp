macro(add_sample dir)
  message(WARNING )
  set(WD ${CMAKE_CURRENT_LIST_DIR}/${dir})
  message(${WD})
  add_test(NAME test-term-sample-${dir} COMMAND ${Python3_EXECUTABLE} ${WD}/run.py $<TARGET_FILE:pepp-term> WORKING_DIRECTORY ${WD})
endmacro()

add_sample(long_strings_error)
