file(GLOB_RECURSE srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Not sharded, because individual tests are cheap. Process start time would
# dominate runtime.
#
# TODO: this should link pepp-core alone but it currently needs pepp-lib + Qt6
# because five of the files reach outside core/.
#
# Pulls in pepp-lib (toolchain):
#
# langs/asmb_pep/codegen.cpp "toolchain/link/mmio.hpp"
# langs/asmb_pep/to_object_code.cpp "toolchain/link/mmio.hpp"
#
# Pulls in Qt6::Core:
#
# io/mapped_file.cpp                  <QDebug> <QString>
# math/bitmanip/strings_ascii_hex.cpp <QList> <qtypes.h>
# langs/asmb/asmb_lexer.cpp           <QtCore>

qt6_add_executable(test-core ${srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(test-core PRIVATE pepp-lib catch Qt6::Core)
set_target_properties(test-core PROPERTIES FOLDER "qtc_runnable")
pepp_precompile_headers(test-core PRIVATE <QtCore>)

if(NOT (EMSCRIPTEN OR IOS))
  add_test(NAME test-core COMMAND test-core)
endif()
