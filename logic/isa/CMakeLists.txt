make_library(isa isa)
target_link_libraries(isa PUBLIC "Qt6::Core")
target_include_directories(isa PUBLIC ${Boost_INCLUDE_DIR})
target_compile_definitions(isa PRIVATE ISA_LIBRARY)

if (NOT IOS)
    #make_qtest(test-bits-copy test/copy.test.cpp "bits;Qt6::Core;Qt6::Test")
    #set_target_properties(test-bits-log2 PROPERTIES FOLDER "qtc_runnable")
endif ()
