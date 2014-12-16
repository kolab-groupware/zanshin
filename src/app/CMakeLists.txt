set(app_SRCS
    aboutdata.cpp
    dependencies.cpp
)

kde4_add_executable(zanshin-next main.cpp ${app_SRCS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
target_link_libraries(zanshin-next
    ${KDE4_KDEUI_LIBS}
    akonadi
    domain
    presentation
    utils
    widgets
    ${KDEPIM_STATIC_LIBS}
)

install(TARGETS zanshin-next DESTINATION ${BIN_INSTALL_DIR})
install(PROGRAMS zanshin-next.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES zanshin.appdata.xml DESTINATION share/appdata)

kde4_add_executable(zanshin-notes notesmain.cpp ${app_SRCS})
target_link_libraries(zanshin-notes
    ${KDE4_KDEUI_LIBS}
    akonadi
    domain
    presentation
    utils
    widgets
    ${KDEPIM_STATIC_LIBS}
)

install(TARGETS zanshin-notes DESTINATION ${BIN_INSTALL_DIR})
