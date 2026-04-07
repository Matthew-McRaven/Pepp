# Do not add_subdirectory(cnl) to avoid including unnecessary tests.
add_library(cnl INTERFACE)

target_compile_features(cnl INTERFACE cxx_std_20)

target_include_directories(cnl
                           INTERFACE "${CMAKE_CURRENT_LIST_DIR}/cnl/include")
