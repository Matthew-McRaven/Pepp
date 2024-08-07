add_subdirectory(preferences)
add_subdirectory(text)
add_subdirectory(memory)
add_subdirectory(cpu)
add_subdirectory(project)


set(ui_targets "text;memory;cpu;prefs;project")


if (APPLE)
    install(TARGETS ${ui_targets} LIBRARY DESTINATION pepp.app/Contents/Frameworks)
else ()
    install(TARGETS ${ui_targets} RUNTIME DESTINATION bin)
endif ()
