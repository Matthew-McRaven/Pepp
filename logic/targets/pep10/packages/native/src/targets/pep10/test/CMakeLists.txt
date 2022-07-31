add_subdirectory(emu)

make_test(pep10-asmb-test asmb)
target_link_libraries(pep10-asmb-test PUBLIC pep10)

make_test(pep10-builtins-test builtins)
target_link_libraries(pep10-builtins-test PUBLIC pep10)

make_test(pep10-elf-test elf)
target_link_libraries(pep10-elf-test PUBLIC pep10)

make_test(pep10-isa-test isa)
target_link_libraries(pep10-isa-test PUBLIC pep10)
