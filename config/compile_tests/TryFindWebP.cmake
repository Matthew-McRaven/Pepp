# Determine if our build environment contains WEBP. Never try to use a user'
# machines non-bundled image formats because I only want to compile one set of
# images into the application. If webp exists at compile time, I can bundle it.
try_run(
  PEPP_WEBP_RETCODE
  PEPP_COMPILES_IMGFMT
  SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/probe_webp.cpp
  LINK_LIBRARIES
  Qt6::Gui
  RUN_OUTPUT_STDOUT_VARIABLE
  PEPP_WEBP_RUN_STDOUT
  RUN_OUTPUT_STDERR_VARIABLE
  PEPP_WEBP_RUN_STDERR
  COMPILE_OUTPUT_VARIABLE PEPP_WEBP_COMPILE_LOG)
if(PEPP_WEBP_RETCODE EQUAL 0)
  message("Using webp for help images")
  add_compile_definitions(PEPP_HAS_WEBP)
  set(PEPP_HAS_WEBP TRUE)
else()
  message("Using png for help images")
endif()
