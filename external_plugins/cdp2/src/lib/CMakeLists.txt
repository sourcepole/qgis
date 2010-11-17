#############################################################
# sources

SET(CDP_CORE_SRCS
  climatedataprocessorcontroller.cpp
  climatedataprocessor.cpp
  climatefilegroup.cpp
  climatefilereader.cpp
  climateimporter.cpp
  filewriter.cpp
  meridianswitcher.cpp
  )
SET(CDP_CORE_MOC_HDRS
  cdp.h
  climatedataprocessorcontroller.h
  climatedataprocessor.h
  climatefilegroup.h
  climatefilereader.h
  climateimporter.h
  filewriter.h
  meridianswitcher.h
  )


QT4_WRAP_CPP(CDP_CORE_MOC_SRCS ${CDP_CORE_MOC_HDRS})

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../../
  ${CMAKE_CURRENT_BINARY_DIR}/../ui/
  ${CMAKE_CURRENT_SOURCE_DIR}/../../ #for cdpversion.h
  ${GDAL_INCLUDE_DIR}
  ${QT_INCLUDE_DIR}
)

#############################################################
# library


ADD_LIBRARY(cdp_core SHARED ${CDP_CORE_SRCS} ${CDP_CORE_MOC_SRCS} )

# because of htonl
IF (WIN32)
  SET(PLATFORM_LIBRARIES wsock32)
ENDIF (WIN32)

IF (APPLE)
  
  SET(PLATFORM_LIBRARIES "-framework ApplicationServices")
ENDIF (APPLE)

TARGET_LINK_LIBRARIES(cdp_core
  ${GDAL_LIBRARY}
  ${QT_QTCORE_LIBRARY} 
  ${QT_QTGUI_LIBRARY} 
  ${QT_QTXML_LIBRARY} 
  ${QT_QTNETWORK_LIBRARY} 
  ${QT_QTSQL_LIBRARY} 
)

INSTALL(TARGETS cdp_core
  RUNTIME DESTINATION ${CDP_LIB_DIR}
  LIBRARY DESTINATION ${CDP_LIB_DIR})

IF (WIN32)
  # expect that classes are being EXPORTED for the lib
  IF (MSVC)
    ADD_DEFINITIONS("-DCDP_LIB_EXPORT=__declspec(dllexport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("-UCDP_LIB_EXPORT \"-DCDP_LIB_EXPORT=__declspec(dllexport)\"")
  ENDIF (MSVC)
ENDIF (WIN32)

#make sure ui are compiled first
ADD_DEPENDENCIES (cdp_core ui)
# Added by Tim to install headers

#INSTALL(CODE "MESSAGE(\"Installing CORE headers...\")")
#INSTALL(FILES ${CDP_CORE_HDRS} ${CDP_CORE_MOC_HDRS} DESTINATION ${CDP_INCLUDE_DIR})
#SUBDIRS(soap)
