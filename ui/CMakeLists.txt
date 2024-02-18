add_subdirectory(highlight)
add_subdirectory(memory)

set(ui_targets "memory;highlight")


if(APPLE)
    install(TARGETS ${ui_targets} LIBRARY DESTINATION pepp.app/Contents/Frameworks)
else()
    install(TARGETS ${ui_targets} RUNTIME DESTINATION bin)
endif()
