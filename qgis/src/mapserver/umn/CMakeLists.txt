#
# Find deps
#
#TODO: gd, freetype, libxml, ...

ADD_DEFINITIONS(-DHAVE_VSNPRINTF -DNEED_STRLCAT -DNEED_STRRSTR -DUSE_FASTCGI -DUSE_WMS_LYR -DUSE_WFS_LYR -DUSE_LIBXML2 -DUSE_CURL -DUSE_WCS_SVR -DUSE_MING_FLASH -DUSE_POSTGIS -DPOSTGIS_HAS_SERVER_VERSION -DUSE_GDAL -DUSE_OGR -DUSE_GEOS -DGEOS_HAS_SIMPLIFY -DUSE_THREAD -DUSE_PROJ -DUSE_EPPL -DUSE_RGBA_PNG -DUSE_AGG -DUSE_GD_GIF -DUSE_GD_PNG -DUSE_GD_JPEG -DUSE_GD_WBMP -DUSE_GD_FT -DGD_HAS_FTEX_XSHOW -DGD_HAS_GDIMAGEGIFPTR -DGD_HAS_GETBITMAPFONTS -DUSE_ICONV -DUSE_ZLIB)

########################################################
# Files

SET ( libmapserver_SRCS
  renderers/agg/src/agg_arc.cpp
  renderers/agg/src/agg_curves.cpp
  renderers/agg/src/agg_vcgen_contour.cpp
  renderers/agg/src/agg_vcgen_dash.cpp
  renderers/agg/src/agg_vcgen_stroke.cpp
  renderers/agg/src/agg_image_filters.cpp
  renderers/agg/src/agg_line_aa_basics.cpp
  renderers/agg/src/agg_line_profile_aa.cpp
  renderers/agg/src/agg_sqrt_tables.cpp
  renderers/agg/src/agg_embedded_raster_fonts.cpp
  renderers/agg/src/agg_trans_affine.cpp
  renderers/agg/src/agg_vpgen_clip_polygon.cpp
  renderers/agg/src/agg_vpgen_clip_polyline.cpp
  renderers/agg/src/agg_font_freetype.cpp
  mapogl.cpp
  mapoglrenderer.cpp
  mapoglcontext.cpp
  mapimageio.c
  mapcairo.c
  maprendering.c
  mapgeomtransform.c
  maprgbapng.c
  maptemplate.c
  mapbits.c
  maphash.c
  mapshape.c
  mapxbase.c
  mapparser.c
  maplexer.c
  maptree.c
  mapsearch.c
  mapstring.c
  mapsymbol.c
  mapfile.c
  maplegend.c
  maputil.c
  mapscale.c
  mapquery.c
  maplabel.c
  maperror.c
  mapprimitive.c
  mapproject.c
  mapraster.c
  mapsde.c
  mapogr.cpp
  mappostgis.c
  mapmygis.c
  maplayer.c
  mapresample.c
  mapwms.c
  mapwmslayer.c
  maporaclespatial.c
  mapgml.c
  mapprojhack.c
  mapthread.c
  mapdraw.c
  mapgd.c
  mapagg.cpp
  mapoutput.c
  mapgdal.c
  mapswf.c
  mapimagemap.c
  mapows.c
  mapwfs.c
  mapwfs11.c
  mapwfslayer.c
  mapcontext.c
  maphttp.c
  mapdrawgdal.c
  mappdf.c
  mapjoin.c
  mapgraticule.c
  mapcopy.c
  mapogcfilter.c
  mapogcsld.c
  maptime.c
  mapwcs.c
  mapwcs11.c
  mapcpl.c
  cgiutil.c
  maprasterquery.c
  mapobject.c
  mapgeos.c
  classobject.c
  layerobject.c
  mapio.c
  mappool.c
  mapsvg.c
  mapregex.c
  mappluginlayer.c
  mapogcsos.c
  mappostgresql.c
  mapcrypto.c
  mapowscommon.c
  maplibxml2.c
  mapdebug.c
  mapchart.c
  maptclutf.c
  epplib.c
  )

INCLUDE_DIRECTORIES(
     ${GDAL_INCLUDE_DIR}
     ${FCGI_INCLUDE_DIR}
     ${GEOS_INCLUDE_DIR}
     ${PROJ_INCLUDE_DIR}
     ${POSTGRES_INCLUDE_DIR}
     ${CMAKE_CURRENT_BINARY_DIR}
     /usr/include/freetype2
     /usr/include/libxml2
     .
)

IF (WITH_INTERNAL_SPATIALITE)
  INCLUDE_DIRECTORIES(BEFORE ../core/spatialite/headers/spatialite)
ENDIF (WITH_INTERNAL_SPATIALITE)

ADD_LIBRARY(mapserver ${libmapserver_SRCS})
