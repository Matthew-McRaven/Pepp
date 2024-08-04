add_subdirectory(preferences)
add_subdirectory(utils)
add_subdirectory(text)
add_subdirectory(help/)
add_subdirectory(memory)
add_subdirectory(cpu)
add_subdirectory(project)


set(ui_targets "ui_utils;text;memory;cpu;prefs;project;ui_help")


if (APPLE)
    install(TARGETS ${ui_targets} LIBRARY DESTINATION pepp.app/Contents/Frameworks)
else ()
    install(TARGETS ${ui_targets} RUNTIME DESTINATION bin)
endif ()
