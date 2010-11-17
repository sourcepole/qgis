## Once run this will define: 
## 
## QGIS_FOUND       = system has QGIS lib
##
## QGIS_LIBRARY     = full path to the library
##
## QGIS_INCLUDE_DIR      = where to find headers 
##
## Tim Sutton


#MESSAGE("Searching for QGIS")
IF(WIN32)
  #MESSAGE("Searching for QGIS in C:/program files/Quantum GIS")

  SET(QGIS_WIN32_PATH "C:/program files/Quantum GIS/include" )
  FIND_PATH(QGIS_INCLUDE_DIR qgsapplication.h "C:/program files/Quantum GIS/include" )
  FIND_LIBRARY(QGIS_LIBRARY NAMES ggis_core PATHS "C:/program files/Quantum GIS/" )

ELSE(WIN32)
  IF(UNIX) 

    # try to use bundle on mac
    IF (APPLE)
      #MESSAGE("Searching for QGIS in /Applications/QGIS.app/Contents/MacOS")
      SET (QGIS_MAC_PATH /Applications/QGIS.app/Contents/MacOS)
      # set INCLUDE_DIR to prefix+include
      SET(QGIS_INCLUDE_DIR ${QGIS_MAC_PATH}/include/qgis CACHE STRING INTERNAL)
      ## extract link dirs 
      SET(QGIS_LIB_DIR ${QGIS_MAC_PATH}/lib CACHE STRING INTERNAL)
      IF (APPLE)
        SET(QGIS_LIBRARY ${QGIS_LIB_DIR}/libqgis_core.dylib ${QGIS_LIB_DIR}/libqgis_gui.dylib CACHE STRING INTERNAL)
      ELSEIF (APPLE)
        SET(QGIS_LIBRARY ${QGIS_LIB_DIR}/libqgis_core.so ${QGIS_LIB_DIR}/libqgis_gui.so CACHE STRING INTERNAL)
      ENDIF (APPLE)
    ELSE (APPLE)
      #MESSAGE("Searching for QGIS in /usr/bin; /usr/local/bin")
      FIND_PROGRAM(QGIS qgis
        /usr/local/bin/
        /usr/bin/
        )
      #MESSAGE("DBG QGIS ${QGIS}")
      IF (QGIS) 
        ## remove suffix om_console because we need the pure directory
        IF (UNIX AND NOT APPLE)
          STRING(REGEX REPLACE "/bin/qgis" "" QGIS_PREFIX ${QGIS} )
        ELSE (UNIX AND NOT APPLE)
          STRING(REGEX REPLACE "/qgis" "" QGIS_PREFIX ${QGIS} )
        ENDIF (UNIX AND NOT APPLE)
        # set INCLUDE_DIR to prefix+include
        SET(QGIS_INCLUDE_DIR ${QGIS_PREFIX}/include/qgis CACHE STRING INTERNAL)
        ## extract link dirs 
        SET(QGIS_LIB_DIR ${QGIS_PREFIX}/lib CACHE STRING INTERNAL)
        SET(QGIS_LIBRARY ${QGIS_LIB_DIR}/libqgis_core.so ${QGIS_LIB_DIR}/libqgis_gui.so CACHE STRING INTERNAL)
      ELSE(QGIS)
        MESSAGE("FindQGIS.cmake: QGIS not found. Please set it manually.")
      ENDIF(QGIS)
    ENDIF (APPLE)
  ENDIF(UNIX)
ENDIF(WIN32)


IF (QGIS_INCLUDE_DIR AND QGIS_LIBRARY)
   SET(QGIS_FOUND TRUE)
ENDIF (QGIS_INCLUDE_DIR AND QGIS_LIBRARY)

IF (QGIS_FOUND)
   IF (NOT QGIS_FIND_QUIETLY)
     MESSAGE(STATUS "Found QGIS: ${QGIS_LIBRARY}")
   ENDIF (NOT QGIS_FIND_QUIETLY)
ELSE (QGIS_FOUND)
   IF (QGIS_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find QGIS")
   ENDIF (QGIS_FIND_REQUIRED)
ENDIF (QGIS_FOUND)
