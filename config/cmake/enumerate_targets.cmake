macro(enumerate_targets OUT_VAR DIR)
    get_property(TGTS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)

    foreach(TGT IN LISTS TGTS)
      LIST(APPEND ${OUT_VAR} ${TGT})
    endforeach()

    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR IN LISTS SUBDIRS)
      enumerate_targets(${OUT_VAR} "${SUBDIR}")
    endforeach()
endmacro()
