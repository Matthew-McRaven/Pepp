cmake_minimum_required(VERSION 3.24)

# Needed so GENERATED is propogated See:
# https://cmake.org/cmake/help/latest/policy/CMP0118.html Discussion here:
# https://discourse.cmake.org/t/bug-with-generated-file-and-target-sources-private/5262/11
# Needs CMake 3.20, hence line 1.
cmake_policy(SET CMP0118 NEW)

include(FetchContent)

option(BUILD_SHARED_LIBS
       "Build using shared libraries. May be ignored on some platforms" ON)

# Allow normalized paths (e.g., "toolchain/pas" ) without nesting that directory
# folder inside a nested "src" hierarchy. May cause linking issues if you do not
# pay attention.
include_directories(${CMAKE_CURRENT_LIST_DIR}/logic
                    ${CMAKE_CURRENT_LIST_DIR}/ui ${CMAKE_CURRENT_LIST_DIR}/lib)

option(SANITIZE "Enable sanitizers" OFF)
if(SANITIZE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-fsanitize=undefined -fsanitize=address)
  add_link_options(-fsanitize=undefined -fsanitize=address)
endif()

# DO NOT AUTO-FORMAT. CI looks for a line with this format to extract the
# version.
# cmake-format: off
project(Pepp VERSION 0.14.2 LANGUAGES C CXX)
# cmake-format: on
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Only overwrite the install prefix if it is the default value.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
      "${PROJECT_BINARY_DIR}/install"
      CACHE PATH "Installation Directory" FORCE)
endif()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Use to all output object files (applications, libraries, etc) in a single
# directory. Without this, Windows does a bad job of copying DLLs on change for
# the terminal application. However, it confuses iOS builds and must be disabled
# there.
if(NOT IOS)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
endif()

if(EMSCRIPTEN)
  add_compile_options(-O1 -gsource-map)
  add_link_options(-sALLOW_MEMORY_GROWTH=1 -sTOTAL_MEMORY=1GB
                   -sINITIAL_MEMORY=150MB -g2 -gsource-map)
  set(QT_WASM_INITIAL_MEMORY
      150MB
      CACHE INTERNAL "" FORCE)
  set(BUILD_SHARED_LIBS FALSE)
endif()

message("Performing build")
set(PROJECT_DATA_DIR "${CMAKE_CURRENT_LIST_DIR}/data")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
# We always want universal builds, so do not set on a per-target basis
find_package(
  Qt6 6.9
  COMPONENTS
  REQUIRED
  Svg
  Quick
  Core
  Gui
  Qml
  Test
  Widgets
  QuickControls2
  Xml
  Sql)
qt_policy(SET QTP0003 NEW) # Use BUILD_SHARED_LIBS as the default library type
                           # for Qt.

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8)
  # Qt6.8/WASM stopped building without this flag being set.
  if(EMSCRIPTEN)
    add_link_options(--bind)
  endif()
  qt_policy(SET QTP0001 NEW)
  qt_policy(SET QTP0004 NEW)
endif()
# 0 is TRACE, which enables all logging in every location. Only works if set as
# a global compile option and before 3rd-party. Default to 3, since I am tired
# of seeing all of my  info messages.
add_compile_options(-DSPDLOG_ACTIVE_LEVEL=3)
add_subdirectory(3rd-party)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_testing()
# Needed to avoid exporting every symbol in every DLL on Windows. Some corner
# cases still require we export symbols manually.
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# Test Qt & STL feature availability
include(config/cmake/create_targets.cmake)
include(config/compile_tests/TryBuildStdRanges.cmake)
include(config/compile_tests/TryFindWebP.cmake)

# Needed to generate documentation, changelogs
find_package(Python COMPONENTS Interpreter)

include(config/cmake/GenerateChangelog.cmake)
include(config/cmake/FindRISCVToolchain.cmake)
inject_cxx_standard()
inject_clang_tidy()
inject_dwarf_debug()
inject_code_coverage()
add_subdirectory(core)
# So that you can #include "core/.." without having a nested core/core folder.
target_include_directories(pepp-core PUBLIC ${CMAKE_CURRENT_LIST_DIR}/core)
add_subdirectory(lib)
add_subdirectory(test)
add_subdirectory(test2)
add_subdirectory(bin)

# Configure CPack installer generation.
set(CPACK_PACKAGE_NAME "Pepp")
set(CPACK_PACKAGE_VENDOR "Pepperdine University")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Pepp IDE")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Pepp")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
# Assuming you're targeting multiple platforms, you might want to conditionally
# set the generator
if(WIN32)
  set(CPACK_GENERATOR "IFW") # or "ZIP" for a simple zip archive
  FetchContent_Declare(
    vcredist_x64
    URL "https://aka.ms/vs/17/release/VC_redist.x64.exe"
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_NAME "VC_redist.x64.exe")
  FetchContent_MakeAvailable(vcredist_x64)
  set(VCREDIST14_X64_EXE "${vcredist_x64_SOURCE_DIR}/VC_redist.x64.exe")
  if(NOT EXISTS "${VCREDIST14_X64_EXE}")
    message(
      FATAL_ERROR "Failed to fetch VC_redist.x64.exe: ${VCREDIST14_X64_EXE}")
  endif()
  # Put the redistributable EXEs into the payload of a component named
  # "vcruntime"
  install(
    FILES "${VCREDIST14_X64_EXE}"
    DESTINATION "vcredist"
    COMPONENT vcruntime)

  # QT IFW could be installed at a couple of locations. Try and detect recent
  # versions first at any location.
  set(VERSIONS "4.10;4.9;4.8;4.7;4.6")
  set(BASE_PATHS "${Qt6_DIR}/../../../../Tools/QtInstallerFramework"
                 "${Qt6_DIR}/../../../../../Tools/QtInstallerFramework")
  foreach(ver IN LISTS VERSIONS)
    foreach(base IN LISTS BASE_PATHS)
      set(candidate "${base}/${ver}")
      cmake_path(NORMAL_PATH candidate OUTPUT_VARIABLE candidate_norm)
      if(IS_DIRECTORY "${candidate_norm}")
        set(CPACK_IFW_ROOT "${candidate_norm}")
        set(_found TRUE) # break out of both loops
        break()
      endif()
    endforeach()
    if(_found)
      break()
    endif()
  endforeach()
  message("Using IFW at: ${CPACK_IFW_ROOT}")

  set(CPACK_IFW_PRODUCT_URL "https://github.com/Matthew-McRaven/Pepp/")
  if(EXISTS "${PROJECT_DATA_DIR}/app_icon/icon.ico")
    set(CPACK_IFW_PACKAGE_ICON "${PROJECT_DATA_DIR}/app_icon/icon.ico")
  endif()
  if(EXISTS "${PROJECT_DATA_DIR}/app_icon/icon.png")
    set(CPACK_IFW_PACKAGE_WINDOW_ICON "${PROJECT_DATA_DIR}/app_icon/icon.png")
  endif()

  set(INSTALLER_SCRIPT
      "${CMAKE_CURRENT_LIST_DIR}/config/installer/installscript.qs")
  if(NOT EXISTS "${INSTALLER_SCRIPT}")
    message(FATAL_ERROR "Installer script file not found!")
  else()
    message("Using installer script: ${INSTALLER_SCRIPT}")
  endif()

  include(CPackIFW)
  cpack_add_component(
    vcruntime
    DISPLAY_NAME "Microsoft Visual C++ 2015–2022 Redistributable" NAME vcruntime
    REQUIRED)
  cpack_add_component(
    Pepp
    DISPLAY_NAME "Pepp IDE" NAME "Pepp"
    REQUIRED)

  # For single component projects, must set CPACK_COMPONENTS_ALL per this issue,
  # else SCRIPT is unset https://gitlab.kitware.com/cmake/cmake/-/issues/22528
  set(CPACK_COMPONENTS_ALL Pepp vcruntime)
  cpack_ifw_configure_component(
    vcruntime FORCED_INSTALLATION REQUIRES_ADMIN_RIGHTS
    NAME "vcruntime"
    DISPLAY_NAME "Microsoft Visual C++ 2015–2022 Redistributable"
    SCRIPT "${CMAKE_CURRENT_LIST_DIR}/config/installer/vcredist.qs")
  cpack_ifw_configure_component(
    Pepp FORCED_INSTALLATION REQUIRES_ADMIN_RIGHTS
    NAME "pepp"
    DISPLAY_NAME "Pepp IDE"
    LICENSES "GPL-3.0" "${PROJECT_SOURCE_DIR}/data/about/dep/pepp-gpl.txt"
    SCRIPT "${INSTALLER_SCRIPT}"
    DEPENDS "vcruntime")

elseif(APPLE)
  # Need monolithic install to keep directory structure stable.
  set(CPACK_MONOLITHIC_INSTALL on)
  set(CPACK_GENERATOR "DragNDrop") # for a macOS .dmg
  set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE TRUE)
  install(
    FILES "${CMAKE_SOURCE_DIR}/LICENSE"
    DESTINATION .
    RENAME "LICENSE.txt")
  # Will still need to notarize & staple the DMG in CI.
  if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message("Mac OS architecture: arm64")
    set(PACKAGE_SUFFIX "-mac-arm64")
  elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
    message("Mac OS architecture: x86_64")
    set(PACKAGE_SUFFIX "-mac-x86")
  else()
    message("Mac OS architecture: unknown")
    set(PACKAGE_SUFFIX "-mac-universal")
  endif()
  set(CPACK_PACKAGE_FILE_NAME
      "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}${PACKAGE_SUFFIX}")
  set(DIR_TO_SIGN
      "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}${PACKAGE_SUFFIX}")
else()
  set(CPACK_GENERATOR "TGZ") # for Linux, multiple generators can be specified
endif()
include(CPack)

# If MAC_DEVELOPER_NAME is not set, try and pick it up from environment
# variable.
set(MAC_DEVELOPER_NAME
    ""
    CACHE STRING "A variable passed in via -D")
if(NOT MAC_DEVELOPER_NAME)
  set(MAC_DEVELOPER_NAME $ENV{MAC_DEVELOPER_NAME})
endif()

# Perform signing in main script (not bin script), because we require all CPACK
# variables to be already defined.
if(APPLE)
  find_program(CODESIGN codesign)
  if(NOT MAC_DEVELOPER_NAME)
    message(WARNING "MAC_DEVELOPER_NAME not set, skipping code signing")
  elseif(CODESIGN_NOTFOUND)
    message(WARNING "codesign not found, skipping code signing")
  else()
    message("Signing app as ${MAC_DEVELOPER_NAME}")
  endif()
  if(CODESIGN AND MAC_DEVELOPER_NAME)
    string(STRIP "${MAC_DEVELOPER_NAME}" MAC_DEVELOPER_NAME)
    set(BODY
        "${CODESIGN} --entitlements ${CMAKE_CURRENT_LIST_DIR}/bin/ide/Entitlements.plist -s ${MAC_DEVELOPER_NAME} -fvvv --deep --timestamp --options=runtime"
    )
    install(
      CODE "
                execute_process(COMMAND ${BODY} ${CMAKE_BINARY_DIR}/_CPack_Packages/Darwin/DragNDrop/${DIR_TO_SIGN}/pepp.app)
            ")
  endif()
endif()
