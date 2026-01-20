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
  LIBRARY DESTINATION pypepp
  RUNTIME DESTINATION pypepp)
  if(MSVC)
    message("Python libs at at ${Python_LIBRARY_DIRS}")
    # Nanobind does not add the %PYTHON_ROOT%\libs directory to the linker path on Windows,
    # which causes an LNK1104: cannot open file 'python3.lib'
    target_link_directories(pypepp_native PRIVATE "${Python_LIBRARY_DIRS}")
  endif()
else()
  message(WARNING "Python 3.12 not found, 'pypepp' module will not be built.")
endif()
