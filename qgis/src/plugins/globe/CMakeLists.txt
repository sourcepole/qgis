
########################################################
# Files

SET (globe_plugin_SRCS
     globe_plugin.cpp
     globe_plugin_gui.cpp
)

SET (globe_plugin_UIS globe_plugin_guibase.ui)

SET (globe_plugin_MOC_HDRS
     globe_plugin.h
     globe_plugin_gui.h
)

SET (globe_plugin_RCCS  globe_plugin.qrc)

########################################################
# Build

QT4_WRAP_UI (globe_plugin_UIS_H  ${globe_plugin_UIS})

QT4_WRAP_CPP (globe_plugin_MOC_SRCS  ${globe_plugin_MOC_HDRS})

QT4_ADD_RESOURCES(globe_plugin_RCC_SRCS ${globe_plugin_RCCS})

ADD_LIBRARY (globeplugin MODULE ${globe_plugin_SRCS} ${globe_plugin_MOC_SRCS} ${globe_plugin_RCC_SRCS} ${globe_plugin_UIS_H})

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     ../../core ../../core/raster ../../core/renderer ../../core/symbology
     ../../gui
     ..
)

TARGET_LINK_LIBRARIES(globeplugin
  qgis_core
  qgis_gui
)


########################################################
# Install

INSTALL(TARGETS globeplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})
