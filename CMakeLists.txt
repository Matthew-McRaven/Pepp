cmake_minimum_required(VERSION 3.24)

# Needed so GENERATED is propogated
# See: https://cmake.org/cmake/help/latest/policy/CMP0118.html
# Discussion here: https://discourse.cmake.org/t/bug-with-generated-file-and-target-sources-private/5262/11
# Needs CMake 3.20, hence line 1.
cmake_policy(SET CMP0118 NEW)

# Allow normalized paths (e.g., "asm/pas" ) without nesting that directory folder
# inside a nested "src" hierarchy. May cause linking issues if you do not pay attention.
include_directories(${CMAKE_CURRENT_LIST_DIR}/logic ${CMAKE_CURRENT_LIST_DIR}/ui ${CMAKE_CURRENT_LIST_DIR}/lib)

# Can be BUILD, VERSION.
# BUILD: compiles the project normally
# VERSION: output the project version to ${CMAKE_CURRENT_LIST_DIR}/version
set(ONLY "BUILD" CACHE STRING "Only build code/documentation/version")
# Must disable QRC logic if emscripten, because a developer may forget to set this flag.
if (EMSCRIPTEN)
    set(DOC_AUDIENCE "WEB")
    add_compile_options(-O1 -gsource-map)
    add_link_options(-sALLOW_MEMORY_GROWTH=1 -sTOTAL_MEMORY=1000000000 -sINITIAL_MEMORY=999948288 -g2 -gsource-map)
    set(QT_WASM_INITIAL_MEMORY 999948288 CACHE INTERNAL "" FORCE)
endif ()
option(SANITIZE "Enable sanitizers" OFF)
if (SANITIZE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fsanitize=undefined -fsanitize=address)
    add_link_options(-fsanitize=undefined -fsanitize=address)
endif ()

# Only set language one actual builds. Otherwise it may try to find buildtools
# on platforms without support for C++. (e.g., documentation builds).
if (ONLY STREQUAL "BUILD")
    set(LANGUAGES "CXX")
endif ()
project(Pepp VERSION 0.9.1 LANGUAGES ${LANGUAGES})
# Only overwrite the install prefix if it is the default value.
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install" CACHE PATH "Installation Directory" FORCE)
endif ()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Use to all output object files (applications, libraries, etc) in a single directory.
# Without this, Windows does a bad job of copying DLLs on change for the terminal application.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")

if (ONLY STREQUAL "BUILD")
    message("Performing build")
    set(PROJECT_DATA_DIR "${CMAKE_CURRENT_LIST_DIR}/data")
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    # We always want universal builds, so do not set on a per-target basis
    find_package(Qt6 6.9 COMPONENTS REQUIRED Svg Quick Core Gui Qml Test Widgets QuickControls2 Xml Sql)
    if(Qt6_VERSION  VERSION_GREATER_EQUAL 6.8)
      # Qt6.8/WASM stopped building without this flag being set.
      if(EMSCRIPTEN)
        add_link_options(--bind)
      endif()
      # Silence Qt's CMake warnings about policy being unset.
      qt_policy(SET QTP0004 NEW)
    endif()

    add_subdirectory(3rd-party)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    enable_testing()
    # Needed to avoid exporting every symbol in every DLL on Windows.
    # Some corner cases still require we export symbols manually.
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    include(config/cmake/create_targets.cmake)
    # Must be earlier or we have issues creating installers.
    try_compile(PEPP_HAS_RANGES_REVERSE ${CMAKE_BINARY_DIR}/compile_tests ${CMAKE_CURRENT_LIST_DIR}/config/compile_tests/ranges_reverse.cpp)
    if(PEPP_HAS_RANGES_REVERSE)
       message("Using std::ranges::reverse for byteswap")
       add_compile_definitions(PEPP_HAS_RANGES_REVERSE)
    else()
       message("Using Qt helpers for byteswap")
    endif()
    # Needed to generate documentation, changelogs
    find_package(Python COMPONENTS Interpreter)

    add_subdirectory(lib)
    add_subdirectory(test)
    add_subdirectory(bin)

    option(FORCE_DISABLE_CHANGEDB "Do not generate changelog DB, even if python is found" OFF)
    # Convert changelog sources to DB, embded in QRC, and inject into library.
    # Do not enable feature if lacking python.
    if(Python_FOUND AND (NOT FORCE_DISABLE_CHANGEDB))
        message("Generating changelog")
        # Ensure that changelog exists at configure time, otherwise qt_add_resources will fail on some Mac platforms.
        execute_process(
            OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/changelog.db
            COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/changelog_tools.py to_sql ${CMAKE_CURRENT_BINARY_DIR}/changelog.db
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
        # Ensure that target only rebuils iff the CSV files change using depends.
        add_custom_command(
                OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/changelog.db
                COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/changelog_tools.py to_sql ${CMAKE_CURRENT_BINARY_DIR}/changelog.db
                DEPENDS ${CMAKE_CURRENT_LIST_DIR}/data/changelog/changes.csv ${CMAKE_CURRENT_LIST_DIR}/data/changelog/versions.csv ${CMAKE_CURRENT_LIST_DIR}/scripts/changelog_tools.py
                COMMENT "Generating changelog SQL"
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        )
        qt_add_resources(pepp-lib "changelog"
                PREFIX "changelog"
                BASE ${CMAKE_CURRENT_BINARY_DIR}
                # This will cause issues in IOS, bug we can cross that bridge later.
                # See: https://doc.qt.io/qt-6/qt-add-resources.html#arguments-of-the-target-based-variant
                BIG_RESOURCES
                FILES ${CMAKE_CURRENT_BINARY_DIR}/changelog.db
        )
    else()
        message("Skipping changelog generation")
    endif()

    # Configure CPack installer generation.
    set(CPACK_PACKAGE_NAME "Pepp")
    set(CPACK_PACKAGE_VENDOR "Pepperdine University")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Pepp IDE")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Pepp")
    # Assuming you're targeting multiple platforms, you might want to conditionally set the generator
    if (WIN32)
        set(CPACK_GENERATOR "IFW") # or "ZIP" for a simple zip archive
        set(CPACK_IFW_ROOT "${Qt6_DIR}/../../../../Tools/QtInstallerFramework/4.6")
        set(CPACK_IFW_PRODUCT_URL "https://github.com/Matthew-McRaven/Pepp/")
        if (EXISTS "${PROJECT_DATA_DIR}/app_icon/icon.ico")
            set(CPACK_IFW_PACKAGE_ICON "${PROJECT_DATA_DIR}/app_icon/icon.ico")
        endif ()
        if (EXISTS "${PROJECT_DATA_DIR}/app_icon/icon.png")
            set(CPACK_IFW_PACKAGE_WINDOW_ICON "${PROJECT_DATA_DIR}/app_icon/icon.png")
        endif ()
        include(CPackIFW)
        cpack_add_component(Pepp
                DISPLAY_NAME Pepp
                REQUIRED
        )
        cpack_ifw_configure_component(Pepp
                FORCED_INSTALLATION
                REQUIRES_ADMIN_RIGHTS
                NAME "Pepp"
                LICENSES "GPL-3.0" "${PROJECT_SOURCE_DIR}/LICENSE"
        )
    elseif (APPLE)
        # Need monolithic install to keep directory structure stable.
        set(CPACK_MONOLITHIC_INSTALL on)
        set(CPACK_GENERATOR "DragNDrop") # for a macOS .dmg
        # Will still need to notarize & staple the DMG in CI.
        if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
            message("Mac OS architecture: arm64")
            set(PACKAGE_SUFFIX "-darwin-arm64")
        elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
            message("Mac OS architecture: x86_64")
            set(PACKAGE_SUFFIX "-darwin-x86")
        else()
            message("Mac OS architecture: unknown")
            set(PACKAGE_SUFFIX "-darwin-universal")
        endif()
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}${PACKAGE_SUFFIX}")
        set(DIR_TO_SIGN "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}${PACKAGE_SUFFIX}")
    else ()
        set(CPACK_GENERATOR "TGZ") # for Linux, multiple generators can be specified
    endif ()
    include(CPack)

    # If MAC_DEVELOPER_NAME is not set, try and pick it up from environment variable.
    set(MAC_DEVELOPER_NAME "" CACHE STRING "A variable passed in via -D")
    if (NOT MAC_DEVELOPER_NAME)
        set(MAC_DEVELOPER_NAME $ENV{MAC_DEVELOPER_NAME})
    endif ()

    # Perform signing in main script (not bin script), because we require all CPACK variables to be already defined.
    if (APPLE)
        find_program(CODESIGN codesign)
        if (NOT MAC_DEVELOPER_NAME)
            message(WARNING "MAC_DEVELOPER_NAME not set, skipping code signing")
        elseif (CODESIGN_NOTFOUND)
            message(WARNING "codesign not found, skipping code signing")
        else()
            message("Signing app as ${MAC_DEVELOPER_NAME}")
        endif ()
        if (CODESIGN AND MAC_DEVELOPER_NAME)
            string(STRIP "${MAC_DEVELOPER_NAME}" MAC_DEVELOPER_NAME)
            set(BODY "${CODESIGN} --entitlements ${CMAKE_CURRENT_LIST_DIR}/bin/Entitlements.plist -s ${MAC_DEVELOPER_NAME} -fvvv --deep --timestamp --options=runtime")
            install(CODE "
                execute_process(COMMAND ${BODY} ${CMAKE_BINARY_DIR}/_CPack_Packages/Darwin/DragNDrop/${DIR_TO_SIGN}/pepp.app)
            ")
        endif ()
    endif ()
elseif (ONLY STREQUAL "VERSION")
    # Write version number out to file, so we can compare against git.
    file(WRITE version ${PROJECT_VERSION})
endif ()

