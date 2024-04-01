add_subdirectory(utils)
add_subdirectory(about)
add_subdirectory(text)
add_subdirectory(memory)
add_subdirectory(cpu)

set(ui_targets "ui_utils;ui_about;text;memory;cpu")


if (APPLE)
    install(TARGETS ${ui_targets} LIBRARY DESTINATION pepp.app/Contents/Frameworks)
else ()
    install(TARGETS ${ui_targets} RUNTIME DESTINATION bin)
endif ()
