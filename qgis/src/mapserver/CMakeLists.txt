#
# Find deps
#
FIND_PACKAGE(Fcgi REQUIRED)
IF (NOT FCGI_FOUND)
  MESSAGE (SEND_ERROR "Fast CGI dependency was not found!")
ENDIF (NOT FCGI_FOUND)

SUBDIRS(umn)

ADD_DEFINITIONS(-DDIAGRAMSERVER=1)

IF (CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  ADD_DEFINITIONS(-DQGSMSDEBUG=1)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)

########################################################
# Files

SET ( qgis_mapserv_SRCS
  qgis_map_serv.cpp
  qgsconfigcache.cpp
  qgsconfigparser.cpp
  qgsepsgcache.cpp
  qgsprojectparser.cpp
  qgshttprequesthandler.cpp 
  qgsgetrequesthandler.cpp
  qgssoaprequesthandler.cpp
  qgssldparser.cpp
  qgssldrenderer.cpp
  qgswmsserver.cpp
  qgsmapserviceexception.cpp
  qgsmapserverlogger.cpp
  qgsmslayercache.cpp
  qgsfilter.cpp
  qgssldrule.cpp
  qgsbetweenfilter.cpp
  qgscomparisonfilter.cpp
  qgslogicalfilter.cpp
  qgsftptransaction.cpp
  qgsmslayerbuilder.cpp 
  qgshostedvdsbuilder.cpp
  qgsinterpolationlayerbuilder.cpp
  qgsremoteowsbuilder.cpp
  qgshostedrdsbuilder.cpp
  qgsremotedatasourcebuilder.cpp
  qgssentdatasourcebuilder.cpp
  qgsmsutils.cpp
  qgsumnwrapper.cpp

  ../plugins/diagram_overlay/qgsdiagramcategory.cpp
  ../plugins/diagram_overlay/qgsdiagramfactory.cpp
  ../plugins/diagram_overlay/qgswkndiagramfactory.cpp
  ../plugins/diagram_overlay/qgsbardiagramfactory.cpp
  ../plugins/diagram_overlay/qgspiediagramfactory.cpp
  ../plugins/diagram_overlay/qgssvgdiagramfactory.cpp
  ../plugins/diagram_overlay/qgsdiagramoverlay.cpp
  ../plugins/diagram_overlay/qgsdiagramrenderer.cpp
  )

# SET (qgis_mapserv_UIS 
# none used 
# )

SET (qgis_mapserv_MOC_HDRS
  qgsftptransaction.h
)

SET (qgis_mapserv_RCCS 
  # not used 
  #qgis_mapserv.qrc
  )

QT4_WRAP_UI (qgis_mapserv_UIS_H  ${qgis_mapserv_UIS})

QT4_WRAP_CPP (qgis_mapserv_MOC_SRCS  ${qgis_mapserv_MOC_HDRS})

QT4_ADD_RESOURCES(qgis_mapserv_RCC_SRCS ${qgis_mapserv_RCCS})

ADD_EXECUTABLE(qgis_mapserv.fcgi
  ${qgis_mapserv_SRCS} 
  ${qgis_mapserv_MOC_SRCS} 
  ${qgis_mapserv_RCC_SRCS} 
  ${qgis_mapserv_UIS_H}
  )

INCLUDE_DIRECTORIES(
  ${GDAL_INCLUDE_DIR}
  ${FCGI_INCLUDE_DIR}
  ${GEOS_INCLUDE_DIR}
  ${PROJ_INCLUDE_DIR}
  ${POSTGRES_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT_INCLUDE_DIR}
  ${QGIS_INCLUDE_DIR}
  ../core
  ../core/raster
  ../core/renderer
  ../core/symbology
  ../core/symbology-ng
  ../core/composer
  ../analysis/interpolation
  ../plugins/diagram_overlay
  .
  )

IF (WITH_INTERNAL_SPATIALITE)
  INCLUDE_DIRECTORIES(BEFORE ../core/spatialite/headers/spatialite)
ENDIF (WITH_INTERNAL_SPATIALITE)

TARGET_LINK_LIBRARIES(qgis_mapserv.fcgi
  qgis_core 
  qgis_analysis
  mapserver
  gd
  # -L/usr/lib64 -ljpeg -lfreetype -lz -L/usr/lib64 -lpng -L/usr/lib64 -lz -L/usr/lib64 -lXpm -lX11      -L/usr/lib64 -ljpeg -lfreetype -lz -L/usr/lib64 -lpng -L/usr/lib64 -lz -L/usr/lib64 -lXpm -lX11  -lproj -L/usr/lib64 -ljpeg -L/usr/lib64 -lpng  -L/usr/lib -lgdal1.7.0 -lproj -L/usr/lib -lgeos_c -lodbc -lodbcinst -L/usr/lib -lexpat -L/usr/lib -lxerces-c -lpthread -ljasper -lhdf5 -lmfhdfalt -ldfalt -logdi -lgif -ljpeg -lpng -lnetcdf -lpq -L/usr/lib -lpq -lz -lpthread -lm -lrt -ldl -L/usr/lib -ldap -ldapserver -ldapclient -lcurl -lidn -llber -lldap -lrt -lgssapi_krb5 -lz -lgnutls -lgcrypt -lxml2 -lz -lm -lpthread -L/usr/lib -lspatialite -lcurl -Wl,-Bsymbolic-functions -Wl,-Bsymbolic-functions -rdynamic -L/usr/lib/mysql -lmysqlclient  -L/usr/lib -lpq -lpgport -lxslt -lxml2 -lpam -lssl -lcrypto -lkrb5 -lcom_err -lgssapi_krb5 -lz -lreadline -lcrypt -ldl -lm   -lcurl -Wl,-Bsymbolic-functions  -L/usr/lib -lgeos_c -lpthread -lc -lfcgi -L/usr/lib64 -lz -lxml2  -lm -lstdc++    -o mapserv
  ${PROJ_LIBRARY}
  ${FCGI_LIBRARY}
  ${POSTGRES_LIBRARY}
  ${GDAL_LIBRARY}
)

########################################################
# Install

INSTALL(CODE "MESSAGE(\"Installing mapserver...\")")
INSTALL(TARGETS 
  qgis_mapserv.fcgi 
  DESTINATION ${QGIS_CGIBIN_DIR}
  )
INSTALL(FILES
  admin.sld 
  wms_metadata.xml
  DESTINATION ${QGIS_CGIBIN_DIR}
  )

