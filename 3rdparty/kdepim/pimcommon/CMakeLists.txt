project(pimcommon)
include_directories(
  ${CMAKE_SOURCE_DIR}/libkdepim
  ${CMAKE_CURRENT_SOURCE_DIR}/util/
  ${CMAKE_CURRENT_SOURCE_DIR}/shorturl/
  ${QJSON_INCLUDE_DIR}
)
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )
add_definitions( -DKDEPIM_STATIC_LIBS )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(libpimcommon_acl_SRCS
  acl/aclentrydialog.cpp
  acl/aclmanager.cpp
  acl/aclutils.cpp
  acl/collectionaclpage.cpp
  acl/imapaclattribute.cpp
)

set(libpimcommon_SRCS
  util/pimutil.cpp
  ${libpimcommon_acl_SRCS}
)

macro(add_resource_iface _kcfgFile _ifaceName _className _fileName)
  kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/${_kcfgFile} ${_ifaceName})
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml PROPERTIES INCLUDE "metatype.h")
  qt4_add_dbus_interface(libpimcommon_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_fileName} ${_className}
  )
endmacro()

add_resource_iface(util/imapresource.kcfg org.kde.Akonadi.Imap.Settings ImapSettings imapresourcesettings)

kde4_add_library(zanshinpimcommonstatic STATIC ${libpimcommon_SRCS})

target_link_libraries(zanshinpimcommonstatic
    ${KDEPIMLIBS_KABC_LIBS}
    ${KDEPIMLIBS_AKONADI_CONTACT_LIBS}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDEPIMLIBS_KIMAP_LIBS}
    ${KDEPIMLIBS_KPIMUTILS_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QJSON_LIBRARIES}
    ${KDE4_SOLID_LIBS}
    ${KDE4_KNEWSTUFF3_LIBS}
    ${LibKGAPI2_LIBRARY}
    zanshinkdepimstatic
)

#set_target_properties(pimcommon PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
#install(TARGETS pimcommon ${INSTALL_TARGETS_DEFAULT_ARGS})

