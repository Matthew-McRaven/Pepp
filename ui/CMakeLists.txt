add_subdirectory(utils)
add_subdirectory(text)
add_subdirectory(memory)

set(ui_targets "ui_utils;memory;text")


if (APPLE)
    install(TARGETS ${ui_targets} LIBRARY DESTINATION pepp.app/Contents/Frameworks)
else ()
    install(TARGETS ${ui_targets} RUNTIME DESTINATION bin)
endif ()
