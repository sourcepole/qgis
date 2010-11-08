#############################################################
# sources

SET(CDP_SRCS
  main.cpp 
  climatedataprocessorgui.cpp
  climateconverter.cpp
  )

SET(CDP_MOC_HDRS
  climatedataprocessorgui.h
  climateconverter.h
  )


SET(CDP_RCCS 
    ../resources/resources.qrc
    )

QT4_WRAP_CPP(CDP_MOC_SRCS ${CDP_MOC_HDRS})

# tell cmake to generate resources
QT4_ADD_RESOURCES(CDP_RCC_SRCS ${CDP_RCCS})

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../../
  ${CMAKE_CURRENT_BINARY_DIR}/../ui
  ${QT_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib
  ${CMAKE_CURRENT_SOURCE_DIR}/../widgets
  ${GDAL_INCLUDE_DIR}
  ${GDAL_INCLUDE_DIR}
  ${QGIS_INCLUDE_DIR}
  ${PROJ_INCLUDE_DIR}
  ${GEOS_INCLUDE_DIR}
  )

#############################################################
# application icon
IF (WIN32)
  IF( MINGW )
    # resource compilation for MinGW
    ADD_CUSTOM_COMMAND( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icon.o
      COMMAND windres.exe -I${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_SOURCE_DIR}/cdp.rc 
      -o ${CMAKE_CURRENT_BINARY_DIR}/icon.o )
    SET(CDP_SRCS ${CDP_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/icon.o)
  ENDIF( MINGW )
    SET(CDP_SRCS ${CDP_SRCS} cdp.rc)
  ELSE( MINGW )
  #todo add msvc icon support
ENDIF (WIN32)
#############################################################
# library


# because of htonl
IF (WIN32)
  SET(PLATFORM_LIBRARIES wsock32)
ENDIF (WIN32)

#Now add these generated files to the ADD_EXECUTABLE step
# If this is NOT done, then the ui_*.h files will not be generated
# ditto for moc, rc etc files
# create an executable file named "cdp" from the source 
# files in the variable "CDP_SRCS".
IF( APPLE )
  add_executable(cdp 
               MACOSX_BUNDLE
               ${CDP_SRCS} 
               ${CDP_UIS_H} 
               ${CDP_MOC_SRCS}
               ${CDP_RCC_SRCS}
               )
  # For Mac OS X, the executable must be at the root of the bundle's executable folder
  INSTALL(TARGETS cdp RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
ELSE( NOT APPLE )

  # Remove console if on WIN32
 IF (WIN32)
    add_executable(cdp WIN32
               ${CDP_SRCS} 
               ${CDP_UIS_H} 
               ${CDP_MOC_SRCS}
               ${CDP_RCC_SRCS}
               )
  ELSE (WIN32)
      add_executable(cdp 
               ${CDP_SRCS} 
               ${CDP_UIS_H} 
               ${CDP_MOC_SRCS}
               ${CDP_RCC_SRCS}
               )
  ENDIF (WIN32)
  
ENDIF( APPLE )

IF (APPLE)
  # For Mac OS X, the executable must be at the root of the bundle's executable folder
  SET (CMAKE_INSTALL_NAME_DIR @executable_path/lib)
  INSTALL(TARGETS cdp RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
ELSE (APPLE)
  INSTALL(TARGETS cdp RUNTIME DESTINATION ${CDP_BIN_DIR})
ENDIF (APPLE)

# link the "cdp" target against the Qt libraries. which libraries 
# exactly, is defined by the "include(${QT_USE_FILE})" line above, which sets up this variable.
SET( QT_USE_QTGUI TRUE )

TARGET_LINK_LIBRARIES(cdp
  ${QT_QTGUI_LIBRARY} 
  ${QT_QTCORE_LIBRARY} 
  ${QT_QTXML_LIBRARY} 
  ${QGIS_LIBRARY}
  ${PROJ_LIBRARY}
  ${GEOS_LIBRARY}
  cdp_core
  )
IF (MSVC)
  # Very important or you get all kinds of odd side
  #effects like app crash on start up saying qtgui.dll
  TARGET_LINK_LIBRARIES(cdp qtmain)
ENDIF (MSVC)
SET_TARGET_PROPERTIES(cdp PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib
  INSTALL_RPATH_USE_LINK_PATH true
)


  # Added by Tim to install headers

  #INSTALL(CODE "MESSAGE(\"Installing GUI headers...\")")
  #INSTALL(FILES ${CDP_HDRS} ${CDP_MOC_HDRS} DESTINATION ${CDP_INCLUDE_DIR})

