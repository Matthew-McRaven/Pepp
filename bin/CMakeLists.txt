if(PEPP_BUILD_GUI)
  add_subdirectory(ide)
  # Disable terminal builds on platforms which do not support it.
  if(NOT (EMSCRIPTEN OR IOS))
    add_subdirectory(term)
  endif()

  qt_generate_deploy_qml_app_script(TARGET pepp OUTPUT_SCRIPT deploy_script
                                    NO_UNSUPPORTED_PLATFORM_ERROR)
  install(SCRIPT ${deploy_script} COMPONENT Pepp)
  # Configure CPack installer generation.
  set(CPACK_PACKAGE_NAME "Pepp")
  set(CPACK_PACKAGE_VENDOR "Pepperdine University")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Pepp IDE")
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "Pepp")
  set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
  # Assuming you're targeting multiple platforms, you might want to
  # conditionally set the generator
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
        "${PROJECT_SOURCE_DIR}/config/installer/installscript.qs")
    if(NOT EXISTS "${INSTALLER_SCRIPT}")
      message(FATAL_ERROR "Installer script file not found!")
    else()
      message("Using installer script: ${INSTALLER_SCRIPT}")
    endif()

    include(CPackIFW)
    cpack_add_component(
      vcruntime
      DISPLAY_NAME "Microsoft Visual C++ 2015–2022 Redistributable" NAME
                                                                    vcruntime
      REQUIRED)
    cpack_add_component(
      Pepp
      DISPLAY_NAME "Pepp IDE" NAME "Pepp"
      REQUIRED)

    # For single component projects, must set CPACK_COMPONENTS_ALL per this
    # issue, else SCRIPT is unset
    # https://gitlab.kitware.com/cmake/cmake/-/issues/22528
    set(CPACK_COMPONENTS_ALL Pepp vcruntime)
    cpack_ifw_configure_component(
      vcruntime FORCED_INSTALLATION REQUIRES_ADMIN_RIGHTS
      NAME "vcruntime"
      DISPLAY_NAME "Microsoft Visual C++ 2015–2022 Redistributable"
      SCRIPT "${PROJECT_SOURCE_DIR}/config/installer/vcredist.qs")
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
      FILES "${PROJECT_SOURCE_DIR}/LICENSE"
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

  # Perform signing in main script (not bin script), because we require all
  # CPACK variables to be already defined.
  if(APPLE)
    # If MAC_DEVELOPER_NAME is not set, try and pick it up from environment
    # variable.
    set(MAC_DEVELOPER_NAME
        ""
        CACHE STRING "A variable passed in via -D")
    if(NOT MAC_DEVELOPER_NAME)
      set(MAC_DEVELOPER_NAME $ENV{MAC_DEVELOPER_NAME})
    endif()
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
          "${CODESIGN} --entitlements ${PROJECT_SOURCE_DIR}/bin/ide/Entitlements.plist -s ${MAC_DEVELOPER_NAME} -fvvv --deep --timestamp --options=runtime"
      )
      install(
        CODE "
              execute_process(COMMAND ${BODY} ${CMAKE_BINARY_DIR}/_CPack_Packages/Darwin/DragNDrop/${DIR_TO_SIGN}/pepp.app)
          ")
    endif()
  endif()
endif()
if(PEPP_BUILD_PYTHON)
  add_subdirectory(pypepp)
endif()
