file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Set icon for the application if it exists.
if(EXISTS "${PROJECT_DATA_DIR}/app_icon/icon.ico" AND WIN32)
  list(APPEND sources "${PROJECT_DATA_DIR}/app_icon/res.rc")
elseif(EXISTS "${PROJECT_DATA_DIR}/app_icon/icon.icns" AND APPLE)
  # Name of icns file, not including directory path or extension.
  set(MACOSX_BUNDLE_ICON_FILE "icon")
  set(app_icon_macos "${PROJECT_DATA_DIR}/app_icon/icon.icns")
  set_source_files_properties(${app_icon_macos}
                              PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  list(APPEND sources ${app_icon_macos})
else()
  # Future platform-specific logic for icons.
endif()

qt6_add_executable(pepp ${sources} ${CMAKE_SOURCE_DIR}/data/icons/icons.qrc
                   ${CMAKE_SOURCE_DIR}/data/app_icon/app_icon.qrc)

# Include additional help images
file(GLOB_RECURSE help_image_files CONFIGURE_DEPENDS
     "${PROJECT_DATA_DIR}/help_images/macos/*.png")

qt_add_resources(
  pepp
  "help_images"
  PREFIX
  "/help_images"
  BASE
  "${PROJECT_DATA_DIR}/help_images/macos"
  FILES
  ${help_image_files})

get_target_property(_pepp_lib_type pepp-lib TYPE)
# If using dylib, copy to app frameworks. Static will be compiled in by default.
if(_pepp_lib_type STREQUAL "STATIC_LIBRARY")
  set(EXTRA_LIBS_IF_STATIC pepp-libplugin)
endif()
target_link_libraries(
  pepp
  PRIVATE cli
          catch
          Qt6::Core
          Qt6::Gui
          Qt::Qml
          Qt::Quick
          Qt::QuickControls2
          Qt6::Svg
          Qt6::Xml
          Qt::Widgets
          ${EXTRA_LIBS_IF_STATIC}
          test-lib-all
          pepp-lib
          KDAB::kddockwidgets
          kddockwidgetsplugin)
qt_import_qml_plugins(pepp)
qt_import_plugins(pepp INCLUDE pepp-libplugin kddockwidgetsplugin
                  INCLUDE_BY_TYPE imageformats)

# Must occur before install scripts, otherwise MacOS builds will fail.
set_target_properties(
  pepp
  PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER edu.pepperdine.cslab.pepp
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE
             MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/Info.plist
             WIN32_EXECUTABLE TRUE
             FOLDER "qtc_runnable")

if(WIN32)
  # Non-DLL platforms have empty generator expression, so use more generator
  # magic to only copy DLLs if they exist.
  add_custom_command(
    TARGET pepp
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} ARGS -E
      $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:pepp>>,copy,true>
      $<TARGET_RUNTIME_DLLS:pepp> $<TARGET_FILE_DIR:pepp>
    COMMAND_EXPAND_LISTS)
  # Must specify antlr, because we can't easily install 3rd-party deps.
  install(
    TARGETS pepp antlr4_shared fmt spdlog
    COMPONENT Pepp
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .)
  # If using dylib, copy to app frameworks.
  if(_pepp_lib_type STREQUAL "SHARED_LIBRARY")
    install(
      TARGETS pepp-lib
      COMPONENT Pepp
      RUNTIME DESTINATION bin
      BUNDLE DESTINATION .)
  endif()
elseif(APPLE)
  # Install our pepp application antlr4 isn't copied by any target since it is a
  # 3rd-party dependency. we must copy it here in a platform-dependent way.
  install(
    TARGETS pepp antlr4_shared fmt spdlog
    COMPONENT Pepp
    LIBRARY DESTINATION pepp.app/Contents/Frameworks
    BUNDLE DESTINATION .)

  # If using dylib, copy to app frameworks.
  if(_pepp_lib_type STREQUAL "SHARED_LIBRARY")
    install(
      TARGETS pepp-lib
      COMPONENT Pepp
      LIBRARY DESTINATION pepp.app/Contents/Frameworks)
  endif()
  set_target_properties(pepp PROPERTIES OUTPUT_NAME "Pepp")
elseif(EMSCRIPTEN)
  # Otherwise use default install logic on Linux for now.
  install(
    TARGETS pepp antlr4_shared
    COMPONENT Pepp
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .)
else()
  # Otherwise use default install logic on Linux for now.
  install(
    TARGETS pepp antlr4_shared
    COMPONENT Pepp
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .)
endif()

# Add main GUI module, qml sources, resources, and add demo applications.
# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob
# converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/*.qml")
set(rel_qml_sources, "")
foreach(f_abs IN LISTS abs_qml_sources)
  file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
  list(APPEND rel_qml_sources "${f_rel}")
endforeach()
qt_add_qml_module(
  pepp
  URI
  Pepp
  VERSION
  0.2
  QML_FILES
  ${rel_qml_sources}
  SOURCES
  # Must be set since we overrode OUTPUT_DIRECTORY in library
  IMPORT_PATH
  "${CMAKE_BINARY_DIR}/edu/pepp")
qt6_add_resources(pepp "pepp" PREFIX "/" FILES ${pep10_resource_files})
