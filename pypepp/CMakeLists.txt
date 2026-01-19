if(CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.12 COMPONENTS Interpreter ${DEV_MODULE})
if(Python_FOUND)
  message(STATUS "Python 3.12 found, building 'pypepp' module.")
  nanobind_add_module(pypepp_native NB_STATIC STABLE_ABI dummy.cpp)
  target_compile_definitions(pypepp_native PRIVATE Py_LIMITED_API=0x030C0000)
  set_target_properties(pypepp_native PROPERTIES FOLDER "qtc_runnable")
  install(TARGETS pypepp_native
  LIBRARY DESTINATION pypepp          # macOS/Linux extension
  RUNTIME DESTINATION pypepp          # Windows .pyd
)
else()
  message(WARNING"Python 3.12 not found, 'pypepp' module will not be built.")
endif()
