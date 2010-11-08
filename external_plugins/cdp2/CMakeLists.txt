SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(COMPLETE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
SET(RELEASE_NAME "Preview1")
SET(PROJECT_VERSION ${COMPLETE_VERSION})
PROJECT(cdp${PROJECT_VERSION})

#############################################################
# CMake settings

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.3)

SET(CMAKE_COLOR_MAKEFILE ON)

# set path to additional CMake modules
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_find_rules ${CMAKE_MODULE_PATH})

SET (WITH_EXPERIMENTAL FALSE CACHE BOOL "Determines whether to use experimental features in the code")
SET (ENABLE_TESTS TRUE CACHE BOOL "Build unit tests (use 'make test' to run them")
SET (PEDANTIC TRUE CACHE BOOL "Treat warnings as errors")


#############################################################
# Check if we should enable unit test builds
IF (ENABLE_TESTS)
  ENABLE_TESTING()
  SET (TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/tests/test_data")
  #enable posting to dart server
  INCLUDE (CTest)
  INCLUDE (Dart)
  INCLUDE (${CMAKE_ROOT}/Modules/Dart.cmake)
  # With the above enabled you can run tests and automatically
  # post to the dart server using 
  #
  #  make Experimental
  #
  # see also http://public.kitware.com/Dart/HTML/Client.shtml
ENDIF (ENABLE_TESTS)


#############################################################
# search for dependencies
FIND_PACKAGE(GDAL)
IF (NOT GDAL_FOUND)
  MESSAGE (SEND_ERROR "GDAL could not be found! You need the gdal dev package to compile.")
ENDIF (NOT GDAL_FOUND)

# search for Qt4

SET( QT_USE_QTSQL      TRUE )
SET( QT_USE_QTXML      TRUE )
SET( QT_USE_QTNETWORK  TRUE )

FIND_PACKAGE(Qt4 REQUIRED)

INCLUDE( ${QT_USE_FILE} )

FIND_PROGRAM(QT_LRELEASE_EXECUTABLE
    NAMES lrelease
    PATHS ${QT_BINARY_DIR}
    NO_DEFAULT_PATH
    )

#############################################################
# Enable the use of RPATH so that the executeable
# library search paths are relative to the executeable
#

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

# the RPATH to be used when installing
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


#############################################################
# enable warnings, halt compile on warnings
IF (PEDANTIC AND NOT MSVC)
  ADD_DEFINITIONS( -Wall -Werror )
ENDIF (PEDANTIC AND NOT MSVC)



#############################################################
# platform specific stuff

IF (WIN32)

  IF (MSVC)
    #tell msvc compiler to use main instead of winmain as the
    #application entry point
    SET(QT_USE_QTMAIN TRUE) 
  ENDIF (MSVC)
  SET (CDP_BIN_DIR    ${CMAKE_INSTALL_PREFIX})
  SET (CDP_LIB_DIR    ${CMAKE_INSTALL_PREFIX})
  SET (CDP_DATA_DIR   ${CMAKE_INSTALL_PREFIX})
  SET (CDP_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
  SET (CDP_SOURCE_DIR ${CMAKE_SOURCE_DIR})

  # expect that classes are being imported by default
  # if they need to be exported, they will be exported
  # in the specific dir, e.g in lib
  # Note: MSVC doesn't like when the macros are quotes
  # and MSYS doesn't like them unqouted (bacause of braces)
  IF (MSVC)
    ADD_DEFINITIONS("-DCDP_LIB_EXPORT=__declspec(dllimport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("\"-DCDP_LIB_EXPORT=__declspec(dllimport)\"")
  ENDIF (MSVC)
  # import qgis classes
  IF (MSVC)
    ADD_DEFINITIONS("-DGUI_EXPORT=__declspec(dllimport)")
    ADD_DEFINITIONS("-DCORE_EXPORT=__declspec(dllimport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("\"-DGUI_EXPORT=__declspec(dllimport)\"")
    ADD_DEFINITIONS("\"-DCORE_EXPORT=__declspec(dllimport)\"")
  ENDIF (MSVC)

ELSE (WIN32)
  # On non windows we dont need all those ugly dllexport things... 
  ADD_DEFINITIONS(-DCDP_LIB_EXPORT=)
  ADD_DEFINITIONS(-DGUI_EXPORT=)
  ADD_DEFINITIONS(-DCORE_EXPORT=)
    
  IF (APPLE)
    # for Mac OS X, everything is put inside an application bundle
    SET (CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/)
    # path for library references
    SET (CMAKE_INSTALL_NAME_DIR @executable_path/lib)
    #set info.plist properties on mac
    SET( PROGNAME ${PROJECT_NAME})
    SET( MACOSX_BUNDLE_ICON_FILE cdp.icns)
    SET( MACOSX_BUNDLE_SHORT_VERSION_STRING 1 )
    SET( MACOSX_BUNDLE_VERSION 1 )
    SET( MACOSX_BUNDLE_LONG_VERSION_STRING Version 1 )
    #SET( CMAKE_OSX_ARCHITECTURES ppc;i386 ) #Comment out if not universal binary
    SET( CMAKE_OSX_ARCHITECTURES i386 ) #Comment out if not universal binary
    SET (CDP_LIB_DIR    ${CMAKE_INSTALL_PREFIX}/cdp.app/Contents/MacOS/lib)
    SET (CDP_DATA_DIR   ${CMAKE_INSTALL_PREFIX}/cdp.app/Contents/MacOS/share/cdp)
    SET (CDP_PLUGIN_DIR ${CMAKE_INSTALL_PREFIX}/cdp.app/Contents/MacOS/lib/cdp)
  ENDIF (APPLE)

  SET (CDP_BIN_DIR    ${CMAKE_INSTALL_PREFIX}/bin)
  SET (CDP_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include/cdp")
  SET (CDP_SOURCE_DIR ${CMAKE_SOURCE_DIR})
  IF (UNIX AND NOT APPLE)
    SET (CDP_LIB_DIR    ${CMAKE_INSTALL_PREFIX}/lib)
    SET (CDP_DATA_DIR   ${CMAKE_INSTALL_PREFIX}/share/cdp)
  ENDIF (UNIX AND NOT APPLE)

ENDIF (WIN32)
#Things applying to all platforms:
#where unit test binaries should be installed to and run from
SET (CDP_TEST_DIR   ${CMAKE_INSTALL_PREFIX}/tests)


#############################################################
# create cdpsvnversion.h
# see http://public.kitware.com/pipermail/cmake/2006-July/010299.html

# We only do this if we are in a .svn dir

FIND_FILE(SVN_MARKER entries PATHS ${CMAKE_SOURCE_DIR}/.svn)

IF (NOT SVN_MARKER)
  SET (SVN_MARKER ${CMAKE_SOURCE_DIR}/CMakeLists.txt)  # Dummy file
ENDIF (NOT SVN_MARKER)
# Add a custom command to drive the svn script whenever the svn entries
# file changes.
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake_templates/svnscript.cmake.in_cmake
               ${CMAKE_CURRENT_BINARY_DIR}/svnscript.cmake
	       @ONLY)

ADD_CUSTOM_COMMAND (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cdpsvnversion.h
  DEPENDS ${SVN_MARKER}
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/svnscript.cmake
)

# Add a custom target to drive the custom command.
ADD_CUSTOM_TARGET(svnversion ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cdpsvnversion.h)



#############################################################
# process subdirs

SUBDIRS(src i18n) 


#############################################################
# install stuff

IF (ENABLE_TESTS)
  IF (WIN32)
    FILE (GLOB xmlfiles sample_data/*.xml)
    FILE (GLOB txtfiles sample_data/*.txt)
    INSTALL (FILES ${xmlfiles} ${txtfiles} DESTINATION ${CMAKE_INSTALL_PREFIX}/sample_data)
  ENDIF (WIN32)
ENDIF (ENABLE_TESTS)

#INSTALL (FILES AUTHORS SPONSORS
#         DESTINATION ${CDP_DATA_DIR}/doc)

# manual page... install also on windows?
#INSTALL (FILES qgis.man
#         DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)

############################################################
# Uninstall stuff
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_templates/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

#############################################################
# Enable packaging

INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "cdp duplicate image remover")
SET(CPACK_PACKAGE_VENDOR "Tim Sutton")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\cdp.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} cdp")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\linfiniti.com")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\linfiniti.com")
  SET(CPACK_NSIS_CONTACT "tim@linfiniti.com")
  SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
  #SET(CPACK_STRIP_FILES "cdp")
  #SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
SET(CPACK_PACKAGE_EXECUTABLES "cdp" "cdp")
INCLUDE(CPack)
