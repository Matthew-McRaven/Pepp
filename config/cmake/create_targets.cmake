include_guard()
# Allows us to not use __declspec(dll...) everywhere
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# We always want universal builds, so do not set on a per-target basis
SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "" FORCE)

# Helper to find and enable clang-tidy
macro(inject_clang_tidy)
    # Run clang tidy iff it is installed and on the path.
    # find_program(CLANG_TIDY "clang-tidy")
    # set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,clang-analyzer-*)
endmacro()

# Force our desired C++ standard.
macro(inject_cxx_standard)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED True)
endmacro()


# Add code coverage support, must come before target definition.
macro(inject_code_coverage)
    if (${code_coverage})
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    endif ()
endmacro()


function(make_target2)
  inject_cxx_standard()
  inject_clang_tidy()
  inject_code_coverage()
  set(options )
  set(oneValueArgs TYPE TARGET)
  set(multiValueArgs SOURCES DEPENDS)
  cmake_parse_arguments("MK" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
  if(("TARGET" IN_LIST MK_KEYWORDS_MISSING_VALUES) OR (NOT DEFINED MK_TARGET))
      message(FATAL_ERROR "TARGET not defined ${MK_KEYWORDS_MISSING_VALUES}")
  endif()

  if(!MSVC)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      add_compile_options(-ftime-trace)
  endif()

  # Ensure target type is set, default to public
  if(("TYPE" IN_LIST MK_KEYWORDS_MISSING_VALUES) OR (NOT DEFINED MK_TYPE))
      message(WARNING "TYPE not defined for target ${MK_TARGET}")
      set(TYPE "PUBLIC")
  endif()

  # PUBLIC is not a valid visibility for libraries, so must exclude that TYPE.
  if (MK_TYPE STREQUAL PUBLIC)
      qt6_add_library(${MK_TARGET} ${MK_SOURCES})
  elseif(MK_TYPE STREQUAL EXEC)
      qt6_add_executable(${MK_TARGET} ${MK_SOURCES})
  elseif(MK_TYPE STREQUAL TEST)
      add_executable(${MK_TARGET} ${MK_SOURCES})
      if(WIN32)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${MK_TARGET})
        set_target_properties(${MK_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${MK_TARGET})
        add_custom_command(TARGET ${MK_TARGET} POST_BUILD
          COMMAND ${CMAKE_COMMAND}
          ARGS -E copy $<TARGET_RUNTIME_DLLS:${MK_TARGET}> $<TARGET_FILE_DIR:${MK_TARGET}>
          COMMAND_EXPAND_LISTS
          )
      endif()
      add_test(NAME ${MK_TARGET} COMMAND ${MK_TARGET})
  else ()
      qt6_add_library(${MK_TARGET} ${MK_TYPE} ${MK_SOURCES})
  endif ()
  # Only add dependencies if present
  if( MK_DEPENDS )
    target_link_libraries(${MK_TARGET} PUBLIC ${MK_DEPENDS})
  endif()
endfunction()

# Helper to make a PUBLIC library with cpp sources.
function(make_library2)
    set(options )
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES DEPENDS TESTS TEST_DEPENDS)
    cmake_parse_arguments(ML "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    string(TOUPPER ${ML_TARGET} ML_TARGET_UPPER)
    make_target2(
        TARGET "${ML_TARGET}"
        TYPE "PUBLIC"
        SOURCES ${ML_SOURCES}
        DEPENDS ${ML_DEPENDS}
    )
    target_compile_definitions(${ML_TARGET} PRIVATE "${ML_TARGET_UPPER}_LIBRARY")
    # Make target for each test file
    foreach(TEST_FILE ${ML_TESTS})
        get_filename_component(STEM ${TEST_FILE} NAME_WE)
        make_target2(
            TARGET "test-${ML_TARGET}-${STEM}"
            TYPE "TEST"
            SOURCES ${TEST_FILE}
            DEPENDS Qt6::Test ${ML_DEPENDS} ${ML_TARGET} ${ML_TEST_DEPENDS}
          )
    endforeach()
endfunction()
