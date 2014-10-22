set(_akonaditest_source_dir ${CMAKE_CURRENT_LIST_DIR})

MACRO(ZANSHIN_AKONADI_AUTO_TESTS)
  FOREACH(_testname ${ARGN})
    kde4_add_executable(${_testname} TEST ${_testname}.cpp)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
    target_link_libraries(${_testname} ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_AKONADI_LIBS}
                                       ${KDEPIMLIBS_KCALCORE_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${QT_QTTEST_LIBRARY}
                                       akonadi
                                       domain
                                       utils
                                       ${KDEPIM_STATIC_LIBS}
    )

    get_target_property(_location ${_testname} LOCATION)
    if (WIN32)
      set(_executable ${_location}.bat)
    else (WIN32)
      set(_executable ${_location}.shell)
    endif (WIN32)

    find_program(_testrunner akonaditest)

    set(AKONADITEST_CWD ${CMAKE_CURRENT_SOURCE_DIR})
    set(zanshin-testcommand "${_executable} 2>&1")
    configure_file(${_akonaditest_source_dir}/akonaditest-run.sh.in ${_testname}-run.sh)

    set(akonaditest-run-cmd ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/testenv/config.xml
                            sh ${CMAKE_CURRENT_BINARY_DIR}/${_testname}-run.sh 2> ${CMAKE_CURRENT_BINARY_DIR}/akonaditest-${_testname}.log)

    add_custom_target(${_testname}-run
                      COMMAND ${akonaditest-run-cmd}
                      WORKING_DIRECTORY ${AKONADITEST_CWD}
    )

    add_test(zanshin-${_testname} make ${_testname}-run)
  ENDFOREACH(_testname)
ENDMACRO(ZANSHIN_AKONADI_AUTO_TESTS)
