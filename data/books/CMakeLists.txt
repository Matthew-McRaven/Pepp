file(GLOB_RECURSE cs-res-abs CONFIGURE_DEPENDS
     "${PROJECT_DATA_DIR}/books/cs4e/*" "${PROJECT_DATA_DIR}/books/cs5e/*"
     "${PROJECT_DATA_DIR}/books/cs6e/*")
# Remove generated / debug files from our book resources
list(FILTER cs-res-abs EXCLUDE REGEX "\.DS_Store")
list(FILTER cs-res-abs EXCLUDE REGEX "\.idea")
list(FILTER cs-res-abs EXCLUDE REGEX "__pycache__")
list(FILTER cs-res-abs EXCLUDE REGEX "\..*_cache")
list(FILTER cs-res-abs EXCLUDE REGEX "\.pdf")
set(ce-res "")
foreach(f-abs IN LISTS cs-res-abs)
  file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
  list(APPEND cs-res "${f-rel}")
endforeach()

include(../../config/cmake/CMakeRC.cmake)
cmrc_add_resource_library(pepp_figures ${cs-res})
