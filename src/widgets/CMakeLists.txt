set(widgets_SRCS
    applicationcomponents.cpp
    availablepagesview.cpp
    availablesourcesview.cpp
    datasourcecombobox.cpp
    datasourcedelegate.cpp
    editorview.cpp
    editorwidget.cpp
    filterwidget.cpp
    recurrencewidget.cpp
    itemdelegate.cpp
    messagebox.cpp
    messageboxinterface.cpp
    newpagedialog.cpp
    newpagedialoginterface.cpp
    pageview.cpp
)

qt4_wrap_ui(widgets_SRCS
    filterwidget.ui
    recurrencewidget.ui
    newpagedialog.ui
)

kde4_add_library(widgets STATIC ${widgets_SRCS})
target_link_libraries(widgets ${QT4_QTGUI_LIBRARY} presentation ${KDEPIM_STATIC_LIBS})
