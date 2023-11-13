make_library(highlight highlight)

target_link_libraries(highlight PUBLIC "Qt6::Core;Qt6::Gui;Qt6::Quick;isa")
target_compile_definitions(highlight PRIVATE HIGHLIGHT_LIBRARY)
