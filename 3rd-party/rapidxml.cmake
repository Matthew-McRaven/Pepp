add_library(rapidxml INTERFACE)
target_include_directories(rapidxml INTERFACE ${CMAKE_CURRENT_LIST_DIR}/rapidxml/include)
target_compile_features(rapidxml INTERFACE  cxx_std_20)
set_target_properties(rapidxml PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
