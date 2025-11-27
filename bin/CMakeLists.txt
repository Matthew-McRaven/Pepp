add_subdirectory(ide)
# Disable terminal builds on platforms which do not support it.
if(NOT (EMSCRIPTEN OR IOS))
  add_subdirectory(term)
endif()

qt_generate_deploy_qml_app_script(TARGET pepp OUTPUT_SCRIPT deploy_script
                                  NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script} COMPONENT Pepp)
