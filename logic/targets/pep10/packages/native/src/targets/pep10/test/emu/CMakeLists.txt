
make_test(pep10-bp-test breakpoints)
target_link_libraries(pep10-bp-test PUBLIC pep10)

make_test(pep10-instr-pre-test instructions_preconditions)
target_link_libraries(pep10-instr-pre-test PUBLIC pep10)

file(GLOB files "${CMAKE_CURRENT_LIST_DIR}/instructions/*.cpp")

foreach (file_name ${files})
    # Get just the basename of the file.
    cmake_path(GET file_name STEM test_name)
    # Comma is an invalid char, and it's definitely in the file names...
    string(REGEX REPLACE "," "_" test_name ${test_name})
    # Make a pretty name for the test so I don't have to do string-hacking in my function calls.
    string(CONCAT test_name "pep10-" ${test_name} "-test")

    make_itest(${test_name} ${file_name})
    target_link_libraries(${test_name} PUBLIC pep10)
endforeach ()
