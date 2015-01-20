include_directories( . )

find_program(XSLTPROC_EXECUTABLE xsltproc DOC "Path to the xsltproc executable")
mark_as_advanced(XSLTPROC_EXECUTABLE)

if(XSLTPROC_EXECUTABLE)
  set(Xsltproc_FOUND TRUE)

  # We depend on kdepimlibs, make sure it's found
  if(NOT DEFINED KDEPIMLIBS_DATA_DIR)
    find_package(KdepimLibs REQUIRED)
  endif()

  # Macro to generate a D-Bus interface description from a KConfigXT file
  macro(kcfg_generate_dbus_interface _kcfg _name)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
      ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
      ${_kcfg}
      > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      DEPENDS ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
      ${_kcfg}
      )
  endmacro()
endif()

add_subdirectory(libkdepim)
add_subdirectory(pimcommon)
add_subdirectory(calendarsupport)
