add_subdirectory(highlight)
add_subdirectory(memory)

set(ui_targets "memory;highlight")
install(
        TARGETS ${ui_targets}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION pepp.app/Contents/Frameworks
)

if(APPLE)
    install(TARGETS ${ui_targets} LIBRARY DESTINATION pepp.app/Contents/Frameworks)
endif()
