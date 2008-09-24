SET( QT_USE_QTTEST     TRUE )
#####################################################
# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} 
# binary make output dirs for uics and mocs
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../../
  ${CMAKE_CURRENT_BINARY_DIR}/../ui
# source dirs
  ${CMAKE_CURRENT_SOURCE_DIR}/../../
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib
  ${CMAKE_CURRENT_SOURCE_DIR}/../gui
  ${QT_INCLUDE_DIR}
  ${GDAL_INCLUDE_DIR}
  )

#MESSAGE("Test Data Dir: ${TEST_DATA_DIR}")
ADD_DEFINITIONS(-DTEST_DATA_DIR="\\"${TEST_DATA_DIR}\\"")

#############################################################
# libraries

# because of htonl
IF (WIN32)
  SET(PLATFORM_LIBRARIES wsock32)
ENDIF (WIN32)

# Since the tests are not actually installed, but rather
# run directly from the build/src/tests dir we need to
# ensure the omg libs can be found.
IF (APPLE)
  # For Mac OS X, the executable must be at the root of the bundle's executable folder
  SET (CMAKE_INSTALL_NAME_DIR @executable_path/../lib)
ENDIF (APPLE)


SET (REQUIRED_LIBS 
      ${QT_QTCORE_LIBRARY} 
      ${QT_QTTEST_LIBRARY} 
      ${QT_QTGUI_LIBRARY} 
      ${QT_QTXML_LIBRARY} 
      ${QT_QTSVG_LIBRARY} 
      ${QT_QTNETWORK_LIBRARY} 
      ${QT_QTMAIN_LIBRARY} 
      cdp_core
    ) 
#note for tests we should not include the moc of our
#qtests in the executable file list as the moc is
#directly included in the sources
#and should not be compiled twice. Trying to include
#them in will cause an error at build time 

#############################################################
# sources
# cdp test
SET(CDPTEST_SRCS cdptest.cpp)
SET(CDPTEST_MOC_CPPS cdptest.cpp)
QT4_WRAP_CPP(CDPTEST_MOC_SRCS ${CDPTEST_MOC_CPPS})
ADD_CUSTOM_TARGET(cdptestmoc ALL DEPENDS ${CDPTEST_MOC_SRCS})
ADD_EXECUTABLE(cdptest ${CDPTEST_SRCS})
ADD_DEPENDENCIES(cdptest cdptestmoc)
TARGET_LINK_LIBRARIES(cdptest ${REQUIRED_LIBS})
INSTALL(TARGETS cdptest RUNTIME DESTINATION ${CDP_TEST_DIR})
ADD_TEST(cdp ${CDP_TEST_DIR}/cdptest)

# Put the libs needed into the test dir
#MESSAGE ("Current Binary Dir: ${CMAKE_CURRENT_BINARY_DIR}")
IF (WIN32)
  INSTALL (FILES ${CMAKE_INSTALL_PREFIX}/cdp_core.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
ENDIF (WIN32)

IF (APPLE)
  INSTALL (FILES ${CMAKE_BINARY_DIR}/src/lib/libcdp_core.dylib DESTINATION ${CDP_TEST_DIR}/lib/)
ENDIF (APPLE)
