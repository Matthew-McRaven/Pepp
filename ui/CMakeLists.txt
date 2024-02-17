add_subdirectory(highlight)
add_subdirectory(memory)

install(
        TARGETS memory highlight
        RUNTIME DESTINATION bin
)