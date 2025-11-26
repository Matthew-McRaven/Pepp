add_subdirectory(ide)
add_subdirectory(term)

qt_generate_deploy_qml_app_script(TARGET pepp OUTPUT_SCRIPT deploy_script
                                  NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script} COMPONENT Pepp)
