if(Python_FOUND)
  # Ensure that changelog exists at configure time, otherwise qt_add_resources
  # will fail on some Mac platforms.
  execute_process(
    OUTPUT_FILE ${CMAKE_BINARY_DIR}/changelog.db
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/changelog_tools.py
            to_sql ${CMAKE_BINARY_DIR}/changelog.db
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  # Ensure that target only rebuils iff the CSV files change using depends.
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/changelog.db
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/changelog_tools.py
            to_sql ${CMAKE_BINARY_DIR}/changelog.db
    DEPENDS ${CMAKE_SOURCE_DIR}/data/changelog/changes.csv
            ${CMAKE_SOURCE_DIR}/data/changelog/versions.csv
            ${CMAKE_SOURCE_DIR}/scripts/changelog_tools.py
    COMMENT "Generating changelog SQL"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  # Must use a dummy static library target. Attempting to use this directly in a
  # shared library fails due to missing rules for object code files.
  add_library(pepp-changelog STATIC)
  # -fPIC is required for a static lib to be linked into shared libs on Linux.
  set_property(TARGET pepp-changelog PROPERTY POSITION_INDEPENDENT_CODE ON)
  qt_add_resources(
    pepp-changelog
    "changelog"
    PREFIX
    "changelog"
    BASE
    ${CMAKE_BINARY_DIR}
    # This will cause issues in IOS, bug we can cross that bridge later. See:
    # https://doc.qt.io/qt-6/qt-add-resources.html#arguments-of-the-target-based-variant
    BIG_RESOURCES
    FILES
    ${CMAKE_BINARY_DIR}/changelog.db)
endif()
