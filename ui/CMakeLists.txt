add_subdirectory(text)
add_subdirectory(memory)
add_subdirectory(preferences)

set(ui_targets "memory;text;preferences")


if (APPLE)
    install(TARGETS ${ui_targets} LIBRARY DESTINATION pepp.app/Contents/Frameworks)
else ()
    install(TARGETS ${ui_targets} RUNTIME DESTINATION bin)
endif ()
