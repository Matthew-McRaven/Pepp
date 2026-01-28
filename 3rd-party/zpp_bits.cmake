add_library(zpp_bits INTERFACE zpp_bits/zpp_bits.h)
target_include_directories(zpp_bits INTERFACE zpp_bits)
target_compile_features(zpp_bits INTERFACE  cxx_std_20)
set_target_properties(zpp_bits PROPERTIES
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
