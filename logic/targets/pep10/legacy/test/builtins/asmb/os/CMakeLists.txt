make_test(pep10-builtins-os-test ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(pep10-builtins-os-test PUBLIC pep10)
copy_dbs(pep10-builtins-os-test)
