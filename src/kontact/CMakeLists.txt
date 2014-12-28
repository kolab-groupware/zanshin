set(part_SRCS
    ../app/aboutdata.cpp
    ../app/dependencies.cpp
    part.cpp
)

kde4_add_plugin(zanshin-next_part ${part_SRCS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
target_link_libraries(zanshin-next_part
    ${KDE4_KPARTS_LIBS}
    akonadi
    domain
    presentation
    utils
    widgets
    ${KDEPIM_STATIC_LIBS}
)

install(TARGETS zanshin-next_part DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES zanshin-next_part.desktop DESTINATION ${SERVICES_INSTALL_DIR})

##########################

set(kontact_SRCS
    kontact_plugin.cpp
)

kde4_add_plugin(kontact_zanshinnextplugin ${kontact_SRCS})
target_link_libraries(kontact_zanshinnextplugin ${KDE4_KPARTS_LIBS} ${KDEPIMLIBS_KONTACTINTERFACE_LIBS})

install(TARGETS kontact_zanshinnextplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES zanshin-next_plugin.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kontact)
install(FILES zanshin-next_part.rc DESTINATION ${DATA_INSTALL_DIR}/zanshin)

################### Note part

set(notepart_SRCS
    ../app/aboutdata.cpp
    ../app/dependencies.cpp
    notes_part.cpp
)

kde4_add_plugin(zanshin-notes_part ${notepart_SRCS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
target_link_libraries(zanshin-notes_part
    ${KDE4_KPARTS_LIBS}
    akonadi
    domain
    presentation
    utils
    widgets
    ${KDEPIM_STATIC_LIBS}
)

install(TARGETS zanshin-notes_part DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES zanshin-notes_part.desktop DESTINATION ${SERVICES_INSTALL_DIR})

##########################

set(notes_kontact_SRCS
    notes_kontact_plugin.cpp
)

kde4_add_plugin(kontact_zanshinnotesplugin ${notes_kontact_SRCS})
target_link_libraries(kontact_zanshinnotesplugin ${KDE4_KPARTS_LIBS} ${KDEPIMLIBS_KONTACTINTERFACE_LIBS})

install(TARGETS kontact_zanshinnotesplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES zanshin-notes_plugin.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kontact)
install(FILES zanshin-notes_part.rc DESTINATION ${DATA_INSTALL_DIR}/zanshin)

################### Task part

set(taskpart_SRCS
    ../app/aboutdata.cpp
    ../app/dependencies.cpp
    tasks_part.cpp
)

kde4_add_plugin(zanshin-tasks_part ${taskpart_SRCS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
target_link_libraries(zanshin-tasks_part
    ${KDE4_KPARTS_LIBS}
    akonadi
    domain
    presentation
    utils
    widgets
    ${KDEPIM_STATIC_LIBS}
)

install(TARGETS zanshin-tasks_part DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES zanshin-tasks_part.desktop DESTINATION ${SERVICES_INSTALL_DIR})

##########################

set(tasks_kontact_SRCS
    tasks_kontact_plugin.cpp
)

kde4_add_plugin(kontact_zanshintasksplugin ${tasks_kontact_SRCS})
target_link_libraries(kontact_zanshintasksplugin ${KDE4_KPARTS_LIBS} ${KDEPIMLIBS_KONTACTINTERFACE_LIBS})

install(TARGETS kontact_zanshintasksplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES zanshin-tasks_plugin.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kontact)
install(FILES zanshin-tasks_part.rc DESTINATION ${DATA_INSTALL_DIR}/zanshin)

