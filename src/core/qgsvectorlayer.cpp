/***************************************************************************
                               qgsvectorlayer.cpp
  This class implements a generic means to display vector layers. The features
  and attributes are read from the data store using a "data provider" plugin.
  QgsVectorLayer can be used with any data store for which an appropriate
  plugin is available.
                              -------------------
          begin                : Oct 29, 2003
          copyright            : (C) 2003 by Gary E.Sherman
          email                : sherman at mrcc.com

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*  $Id$ */

#include <cfloat>
#include <cstring>
#include <climits>
#include <cmath>
#include <iosfwd>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <utility>

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QSettings>
#include <QString>
#include <QDomNode>

#include "qgsvectorlayer.h"

// renderers
#include "qgscontinuouscolorrenderer.h"
#include "qgsgraduatedsymbolrenderer.h"
#include "qgsrenderer.h"
#include "qgssinglesymbolrenderer.h"
#include "qgsuniquevaluerenderer.h"

#include "qgsattributeaction.h"

#include "qgis.h" //for globals
#include "qgsapplication.h"
#include "qgscoordinatetransform.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgsgeometry.h"
#include "qgslabel.h"
#include "qgslogger.h"
#include "qgsmaptopixel.h"
#include "qgspoint.h"
#include "qgsproviderregistry.h"
#include "qgsrectangle.h"
#include "qgsrendercontext.h"
#include "qgssinglesymbolrenderer.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayeriterator.h"
#include "qgsvectorlayerundocommand.h"
#include "qgsvectoroverlay.h"
#include "qgsmaplayerregistry.h"
#include "qgsclipper.h"
#include "qgsproject.h"

#include "qgsrendererv2.h"
#include "qgssymbolv2.h"
#include "qgssymbollayerv2.h"
#include "qgssinglesymbolrendererv2.h"

#include "qgsvectorlayereditbuffer.h"

#ifdef TESTPROVIDERLIB
#include <dlfcn.h>
#endif


static const char * const ident_ = "$Id$";

// typedef for the QgsDataProvider class factory
typedef QgsDataProvider * create_it( const QString* uri );



QgsVectorLayer::QgsVectorLayer( QString vectorLayerPath,
                                QString baseName,
                                QString providerKey,
                                bool loadDefaultStyleFlag )
    : QgsMapLayer( VectorLayer, baseName, vectorLayerPath ),
    mDataProvider( NULL ),
    mProviderKey( providerKey ),
    mEditBuffer( NULL ),
    mRenderer( 0 ),
    mRendererV2( NULL ),
    mUsingRendererV2( false ),
    mLabel( 0 ),
    mLabelOn( false ),
    mVertexMarkerOnlyForSelection( false )
{
  mActions = new QgsAttributeAction;

  // if we're given a provider type, try to create and bind one to this layer
  if ( ! mProviderKey.isEmpty() )
  {
    setDataProvider( mProviderKey );
  }
  if ( mValid )
  {
    // Always set crs
    setCoordinateSystem();

    QSettings settings;
    if ( settings.value( "/qgis/use_symbology_ng", false ).toBool() )
    {
      // using symbology-ng!
      setUsingRendererV2( true );
    }

    // check if there is a default style / propertysheet defined
    // for this layer and if so apply it
    bool defaultLoadedFlag = false;
    if ( loadDefaultStyleFlag )
    {
      loadDefaultStyle( defaultLoadedFlag );
    }

    // if the default style failed to load or was disabled use some very basic defaults
    if ( !defaultLoadedFlag )
    {
      // add single symbol renderer
      if ( mUsingRendererV2 )
      {
        setRendererV2( QgsFeatureRendererV2::defaultRenderer( geometryType() ) );
      }
      else
      {
        QgsSingleSymbolRenderer *renderer = new QgsSingleSymbolRenderer( geometryType() );
        setRenderer( renderer );
      }
    }

  }
} // QgsVectorLayer ctor



QgsVectorLayer::~QgsVectorLayer()
{
  QgsDebugMsg( "In QgsVectorLayer destructor" );

  emit layerDeleted();

  mValid = false;

  if ( mRenderer )
  {
    delete mRenderer;
  }
  // delete the provider object
  delete mDataProvider;

  delete mLabel;

  // Destroy any cached geometries and clear the references to them
  deleteCachedGeometries();

  delete mActions;

  //delete remaining overlays

  QList<QgsVectorOverlay*>::iterator overlayIt = mOverlays.begin();
  for ( ; overlayIt != mOverlays.end(); ++overlayIt )
  {
    delete *overlayIt;
  }
}

QString QgsVectorLayer::storageType() const
{
  if ( mDataProvider )
  {
    return mDataProvider->storageType();
  }
  return 0;
}


QString QgsVectorLayer::capabilitiesString() const
{
  if ( mDataProvider )
  {
    return mDataProvider->capabilitiesString();
  }
  return 0;
}

QString QgsVectorLayer::dataComment() const
{
  if ( mDataProvider )
  {
    return mDataProvider->dataComment();
  }
  return QString();
}


QString QgsVectorLayer::providerType() const
{
  return mProviderKey;
}

/**
 * sets the preferred display field based on some fuzzy logic
 */
void QgsVectorLayer::setDisplayField( QString fldName )
{
  // If fldName is provided, use it as the display field, otherwise
  // determine the field index for the feature column of the identify
  // dialog. We look for fields containing "name" first and second for
  // fields containing "id". If neither are found, the first field
  // is used as the node.
  QString idxName = "";
  QString idxId = "";

  if ( !fldName.isEmpty() )
  {
    mDisplayField = fldName;
  }
  else
  {
    const QgsFieldMap &fields = pendingFields();
    int fieldsSize = fields.size();

    for ( QgsFieldMap::const_iterator it = fields.begin(); it != fields.end(); ++it )
    {
      QString fldName = it.value().name();
      QgsDebugMsg( "Checking field " + fldName + " of " + QString::number( fieldsSize ) + " total" );

      // Check the fields and keep the first one that matches.
      // We assume that the user has organized the data with the
      // more "interesting" field names first. As such, name should
      // be selected before oldname, othername, etc.
      if ( fldName.indexOf( "name", false ) > -1 )
      {
        if ( idxName.isEmpty() )
        {
          idxName = fldName;
        }
      }
      if ( fldName.indexOf( "descrip", false ) > -1 )
      {
        if ( idxName.isEmpty() )
        {
          idxName = fldName;
        }
      }
      if ( fldName.indexOf( "id", false ) > -1 )
      {
        if ( idxId.isEmpty() )
        {
          idxId = fldName;
        }
      }
    }

    //if there were no fields in the dbf just return - otherwise qgis segfaults!
    if ( fieldsSize == 0 )
      return;

    if ( idxName.length() > 0 )
    {
      mDisplayField = idxName;
    }
    else
    {
      if ( idxId.length() > 0 )
      {
        mDisplayField = idxId;
      }
      else
      {
        mDisplayField = fields[0].name();
      }
    }

  }
}

// NOTE this is a temporary method added by Tim to prevent label clipping
// which was occurring when labeller was called in the main draw loop
// This method will probably be removed again in the near future!
void QgsVectorLayer::drawLabels( QgsRenderContext& rendererContext )
{
  QgsDebugMsg( "Starting draw of labels" );

  if (( mRenderer || mRendererV2 ) && mLabelOn &&
      ( !label()->scaleBasedVisibility() ||
        ( label()->minScale() <= rendererContext.rendererScale() &&
          rendererContext.rendererScale() <= label()->maxScale() ) ) )
  {
    QgsAttributeList attributes;
    if ( mRenderer )
    {
      attributes = mRenderer->classificationAttributes();
    }
    else if ( mRendererV2 )
    {
      foreach( QString attrName, mRendererV2->usedAttributes() )
      {
        int attrNum = fieldNameIndex( attrName );
        attributes.append( attrNum );
      }
      // make sure the renderer is ready for classification ("symbolForFeature")
      mRendererV2->startRender( rendererContext, this );
    }

    // Add fields required for labels
    mLabel->addRequiredFields( attributes );

    QgsDebugMsg( "Selecting features based on view extent" );

    int featureCount = 0;

    try
    {
      // select the records in the extent. The provider sets a spatial filter
      // and sets up the selection set for retrieval
      select( attributes, rendererContext.extent() );

      QgsFeature fet;
      while ( nextFeature( fet ) )
      {
        if (( mRenderer && mRenderer->willRenderFeature( &fet ) )
            || ( mRendererV2 && mRendererV2->symbolForFeature( fet ) != NULL ) )
        {
          bool sel = mSelectedFeatureIds.contains( fet.id() );
          mLabel->renderLabel( rendererContext, fet, sel, 0 );
        }
        featureCount++;
      }
    }
    catch ( QgsCsException &e )
    {
      Q_UNUSED( e );
      QgsDebugMsg( "Error projecting label locations" );
    }

    if ( mRendererV2 )
    {
      mRendererV2->stopRender( rendererContext );
    }

    QgsDebugMsg( QString( "Total features processed %1" ).arg( featureCount ) );

    // XXX Something in our draw event is triggering an additional draw event when resizing [TE 01/26/06]
    // XXX Calling this will begin processing the next draw event causing image havoc and recursion crashes.
    //qApp->processEvents();

  }
}


unsigned char *QgsVectorLayer::drawLineString( unsigned char *feature, QgsRenderContext &renderContext )
{
  QPainter *p = renderContext.painter();
  unsigned char *ptr = feature + 5;
  unsigned int wkbType = *(( int* )( feature + 1 ) );
  unsigned int nPoints = *(( int* )ptr );
  ptr = feature + 9;

  bool hasZValue = ( wkbType == QGis::WKBLineString25D );

  std::vector<double> x( nPoints );
  std::vector<double> y( nPoints );
  std::vector<double> z( nPoints, 0.0 );

  // Extract the points from the WKB format into the x and y vectors.
  for ( register unsigned int i = 0; i < nPoints; ++i )
  {
    x[i] = *(( double * ) ptr );
    ptr += sizeof( double );
    y[i] = *(( double * ) ptr );
    ptr += sizeof( double );

    if ( hasZValue ) // ignore Z value
      ptr += sizeof( double );
  }

  // Transform the points into map coordinates (and reproject if
  // necessary)

  transformPoints( x, y, z, renderContext );

  // Work around a +/- 32768 limitation on coordinates
  // Look through the x and y coordinates and see if there are any
  // that need trimming. If one is found, there's no need to look at
  // the rest of them so end the loop at that point.
  for ( register unsigned int i = 0; i < nPoints; ++i )
  {
    if ( std::abs( x[i] ) > QgsClipper::MAX_X ||
         std::abs( y[i] ) > QgsClipper::MAX_Y )
    {
      QgsClipper::trimFeature( x, y, true ); // true = polyline
      nPoints = x.size(); // trimming may change nPoints.
      break;
    }
  }

  // set up QPolygonF class with transformed points
  QPolygonF pa( nPoints );
  for ( register unsigned int i = 0; i < nPoints; ++i )
  {
    pa[i].setX( x[i] );
    pa[i].setY( y[i] );
  }

  // The default pen gives bevelled joins between segements of the
  // polyline, which is good enough for the moment.
  //preserve a copy of the pen before we start fiddling with it
  QPen pen = p->pen(); // to be kept original

  //
  // experimental alpha transparency
  // 255 = opaque
  //
  QPen myTransparentPen = p->pen(); // store current pen
  QColor myColor = myTransparentPen.color();
  //only set transparency from layer level if renderer does not provide
  //transparency on class level
  if ( !mRenderer->usesTransparency() )
  {
    myColor.setAlpha( mTransparencyLevel );
  }
  myTransparentPen.setColor( myColor );
  p->setPen( myTransparentPen );
  p->drawPolyline( pa );

  // draw vertex markers if in editing mode, but only to the main canvas
  if ( isEditable() && renderContext.drawEditingInformation() )
  {

    std::vector<double>::const_iterator xIt;
    std::vector<double>::const_iterator yIt;
    for ( xIt = x.begin(), yIt = y.begin(); xIt != x.end(); ++xIt, ++yIt )
    {
      drawVertexMarker( *xIt, *yIt, *p, mCurrentVertexMarkerType, mCurrentVertexMarkerSize );
    }
  }

  //restore the pen
  p->setPen( pen );

  return ptr;
}

unsigned char *QgsVectorLayer::drawPolygon( unsigned char *feature, QgsRenderContext &renderContext )
{
  QPainter *p = renderContext.painter();
  typedef std::pair<std::vector<double>, std::vector<double> > ringType;
  typedef ringType* ringTypePtr;
  typedef std::vector<ringTypePtr> ringsType;

  // get number of rings in the polygon
  unsigned int numRings = *(( int* )( feature + 1 + sizeof( int ) ) );

  if ( numRings == 0 )  // sanity check for zero rings in polygon
    return feature + 9;

  unsigned int wkbType = *(( int* )( feature + 1 ) );

  bool hasZValue = ( wkbType == QGis::WKBPolygon25D );

  int total_points = 0;

  // A vector containing a pointer to a pair of double vectors.The
  // first vector in the pair contains the x coordinates, and the
  // second the y coordinates.
  ringsType rings;

  // Set pointer to the first ring
  unsigned char* ptr = feature + 1 + 2 * sizeof( int );

  for ( register unsigned int idx = 0; idx < numRings; idx++ )
  {
    unsigned int nPoints = *(( int* )ptr );

    ringTypePtr ring = new ringType( std::vector<double>( nPoints ), std::vector<double>( nPoints ) );
    ptr += 4;

    // create a dummy vector for the z coordinate
    std::vector<double> zVector( nPoints, 0.0 );
    // Extract the points from the WKB and store in a pair of
    // vectors.
    for ( register unsigned int jdx = 0; jdx < nPoints; jdx++ )
    {
      ring->first[jdx] = *(( double * ) ptr );
      ptr += sizeof( double );
      ring->second[jdx] = *(( double * ) ptr );
      ptr += sizeof( double );

      if ( hasZValue )
        ptr += sizeof( double );
    }
    // If ring has fewer than two points, what is it then?
    // Anyway, this check prevents a crash
    if ( nPoints < 1 )
    {
      QgsDebugMsg( "Ring has only " + QString::number( nPoints ) + " points! Skipping this ring." );
      continue;
    }

    transformPoints( ring->first, ring->second, zVector, renderContext );

    // Work around a +/- 32768 limitation on coordinates
    // Look through the x and y coordinates and see if there are any
    // that need trimming. If one is found, there's no need to look at
    // the rest of them so end the loop at that point.
    for ( register unsigned int i = 0; i < nPoints; ++i )
    {
      if ( std::abs( ring->first[i] ) > QgsClipper::MAX_X ||
           std::abs( ring->second[i] ) > QgsClipper::MAX_Y )
      {
        QgsClipper::trimFeature( ring->first, ring->second, false );
        break;
      }
    }

    // Don't bother keeping the ring if it has been trimmed out of
    // existence.
    if ( ring->first.size() == 0 )
      delete ring;
    else
    {
      rings.push_back( ring );
      total_points += ring->first.size();
    }
  }

  // Now we draw the polygons

  // use painter paths for drawing polygons with holes
  // when adding polygon to the path they invert the area
  // this means that adding inner rings to the path creates
  // holes in outer ring
  QPainterPath path; // OddEven fill rule by default

  // Only try to draw polygons if there is something to draw
  if ( total_points > 0 )
  {
    //preserve a copy of the brush and pen before we start fiddling with it
    QBrush brush = p->brush(); //to be kept as original
    QPen pen = p->pen(); // to be kept original
    //
    // experimental alpha transparency
    // 255 = opaque
    //
    QBrush myTransparentBrush = p->brush();
    QColor myColor = brush.color();

    //only set transparency from layer level if renderer does not provide
    //transparency on class level
    if ( !mRenderer->usesTransparency() )
    {
      myColor.setAlpha( mTransparencyLevel );
    }
    myTransparentBrush.setColor( myColor );
    QPen myTransparentPen = p->pen(); // store current pen
    myColor = myTransparentPen.color();

    //only set transparency from layer level if renderer does not provide
    //transparency on class level
    if ( !mRenderer->usesTransparency() )
    {
      myColor.setAlpha( mTransparencyLevel );
    }
    myTransparentPen.setColor( myColor );

    p->setBrush( myTransparentBrush );
    p->setPen( myTransparentPen );

    if ( numRings == 1 )
    {
      ringTypePtr r = rings[0];
      unsigned ringSize = r->first.size();

      QPolygonF pa( ringSize );
      for ( register unsigned int j = 0; j != ringSize; ++j )
      {
        pa[j].setX( r->first[j] );
        pa[j].setY( r->second[j] );
      }
      p->drawPolygon( pa );

      // draw vertex markers if in editing mode, but only to the main canvas
      if ( isEditable() && renderContext.drawEditingInformation() )
      {
        for ( register unsigned int j = 0; j != ringSize; ++j )
        {
          drawVertexMarker( r->first[j], r->second[j], *p, mCurrentVertexMarkerType, mCurrentVertexMarkerSize );
        }
      }

      delete rings[0];
    }
    else
    {
      // Store size here and use it in the loop to avoid penalty of
      // multiple calls to size()
      int numRings = rings.size();
      for ( register int i = 0; i < numRings; ++i )
      {
        // Store the pointer in a variable with a short name so as to make
        // the following code easier to type and read.
        ringTypePtr r = rings[i];
        // only do this once to avoid penalty of additional calls
        unsigned ringSize = r->first.size();

        // Transfer points to the array of QPointF
        QPolygonF pa( ringSize );
        for ( register unsigned int j = 0; j != ringSize; ++j )
        {
          pa[j].setX( r->first[j] );
          pa[j].setY( r->second[j] );
        }

        path.addPolygon( pa );

        // Tidy up the pointed to pairs of vectors as we finish with them
        delete rings[i];
      }

#if 0
      // A bit of code to aid in working out what values of
      // QgsClipper::minX, etc cause the X11 zoom bug.
      int largestX  = -std::numeric_limits<int>::max();
      int smallestX = std::numeric_limits<int>::max();
      int largestY  = -std::numeric_limits<int>::max();
      int smallestY = std::numeric_limits<int>::max();

      for ( int i = 0; i < pa.size(); ++i )
      {
        largestX  = std::max( largestX,  pa.point( i ).x() );
        smallestX = std::min( smallestX, pa.point( i ).x() );
        largestY  = std::max( largestY,  pa.point( i ).y() );
        smallestY = std::min( smallestY, pa.point( i ).y() );
      }
      QgsDebugMsg( QString( "Largest  X coordinate was %1" ).arg( largestX ) );
      QgsDebugMsg( QString( "Smallest X coordinate was %1" ).arg( smallestX ) );
      QgsDebugMsg( QString( "Largest  Y coordinate was %1" ).arg( largestY ) );
      QgsDebugMsg( QString( "Smallest Y coordinate was %1" ).arg( smallestY ) );
#endif

      //
      // draw the polygon
      //
      p->drawPath( path );

      // draw vertex markers if in editing mode, but only to the main canvas
      if ( isEditable() && renderContext.drawEditingInformation() )
      {
        for ( int i = 0; i < path.elementCount(); ++i )
        {
          const QPainterPath::Element & e = path.elementAt( i );
          drawVertexMarker( e.x, e.y, *p, mCurrentVertexMarkerType, mCurrentVertexMarkerSize );
        }
      }
    }

    //
    //restore brush and pen to original
    //
    p->setBrush( brush );
    p->setPen( pen );

  } // totalPoints > 0

  return ptr;
}

void QgsVectorLayer::drawRendererV2( QgsRenderContext& rendererContext, bool labeling, QgsFeatureIterator& fi )
{
  QSettings settings;
  bool vertexMarkerOnlyForSelection = settings.value( "/qgis/digitizing/marker_only_for_selected", false ).toBool();

  mRendererV2->startRender( rendererContext, this );

  QgsFeature fet;
  while ( fi.nextFeature( fet ) )
  {
    try
    {
      if ( rendererContext.renderingStopped() )
      {
        break;
      }

      bool sel = mSelectedFeatureIds.contains( fet.id() );
      bool drawMarker = ( isEditable() && ( !vertexMarkerOnlyForSelection || sel ) );

      // render feature
      mRendererV2->renderFeature( fet, rendererContext, -1, sel, drawMarker );

      // labeling - register feature
      if ( labeling && mRendererV2->symbolForFeature( fet ) != NULL )
        rendererContext.labelingEngine()->registerFeature( this, fet, rendererContext );

      if ( isEditable() )
      {
        // Cache this for the use of (e.g.) modifying the feature's uncommitted geometry.
        mCachedGeometries[fet.id()] = *fet.geometry();
      }
    }
    catch ( const QgsCsException &cse )
    {
      QgsDebugMsg( QString( "Failed to transform a point while drawing a feature of type '%1'. Ignoring this feature. %2" )
                   .arg( fet.typeName() ).arg( cse.what() ) );
    }
  }
}

void QgsVectorLayer::drawRendererV2Levels( QgsRenderContext& rendererContext, bool labeling, QgsFeatureIterator& fi )
{
  QHash< QgsSymbolV2*, QList<QgsFeature> > features; // key = symbol, value = array of features

  QSettings settings;
  bool vertexMarkerOnlyForSelection = settings.value( "/qgis/digitizing/marker_only_for_selected", false ).toBool();

  // startRender must be called before symbolForFeature() calls to make sure renderer is ready
  mRendererV2->startRender( rendererContext, this );

  QgsSingleSymbolRendererV2* selRenderer = NULL;
  if ( !mSelectedFeatureIds.isEmpty() )
  {
    selRenderer = new QgsSingleSymbolRendererV2( QgsSymbolV2::defaultSymbol( geometryType() ) );
    selRenderer->symbol()->setColor( QgsRenderer::selectionColor() );
    selRenderer->setVertexMarkerAppearance( currentVertexMarkerType(), currentVertexMarkerSize() );
    selRenderer->startRender( rendererContext, this );
  }

  // 1. fetch features
  QgsFeature fet;
  while ( fi.nextFeature( fet ) )
  {
    if ( rendererContext.renderingStopped() )
    {
      stopRendererV2( rendererContext, selRenderer );
      return;
    }
    QgsSymbolV2* sym = mRendererV2->symbolForFeature( fet );
    if ( !features.contains( sym ) )
    {
      features.insert( sym, QList<QgsFeature>() );
    }
    features[sym].append( fet );

    if ( labeling && mRendererV2->symbolForFeature( fet ) != NULL )
      rendererContext.labelingEngine()->registerFeature( this, fet, rendererContext );

    if ( isEditable() )
    {
      // Cache this for the use of (e.g.) modifying the feature's uncommitted geometry.
      mCachedGeometries[fet.id()] = *fet.geometry();
    }
  }

  // find out the order
  QgsSymbolV2LevelOrder levels;
  QgsSymbolV2List symbols = mRendererV2->symbols();
  for ( int i = 0; i < symbols.count(); i++ )
  {
    QgsSymbolV2* sym = symbols[i];
    for ( int j = 0; j < sym->symbolLayerCount(); j++ )
    {
      int level = sym->symbolLayer( j )->renderingPass();
      QgsSymbolV2LevelItem item( sym, j );
      while ( level >= levels.count() ) // append new empty levels
        levels.append( QgsSymbolV2Level() );
      levels[level].append( item );
    }
  }

  // 2. draw features in correct order
  for ( int l = 0; l < levels.count(); l++ )
  {
    QgsSymbolV2Level& level = levels[l];
    for ( int i = 0; i < level.count(); i++ )
    {
      QgsSymbolV2LevelItem& item = level[i];
      if ( !features.contains( item.symbol() ) )
      {
        QgsDebugMsg( "level item's symbol not found!" );
        continue;
      }
      int layer = item.layer();
      QList<QgsFeature>& lst = features[item.symbol()];
      QList<QgsFeature>::iterator fit;
      for ( fit = lst.begin(); fit != lst.end(); ++fit )
      {
        if ( rendererContext.renderingStopped() )
        {
          stopRendererV2( rendererContext, selRenderer );
          return;
        }
        bool sel = mSelectedFeatureIds.contains( fit->id() );
        // maybe vertex markers should be drawn only during the last pass...
        bool drawMarker = ( isEditable() && ( !vertexMarkerOnlyForSelection || sel ) );

        try
        {
          mRendererV2->renderFeature( *fit, rendererContext, layer, sel, drawMarker );
        }
        catch ( const QgsCsException &cse )
        {
          QgsDebugMsg( QString( "Failed to transform a point while drawing a feature of type '%1'. Ignoring this feature. %2" )
                       .arg( fet.typeName() ).arg( cse.what() ) );
        }
      }
    }
  }

  stopRendererV2( rendererContext, selRenderer );
}

bool QgsVectorLayer::draw( QgsRenderContext& rendererContext )
{
  if ( mUsingRendererV2 )
  {
    if ( mRendererV2 == NULL )
      return false;

    QgsDebugMsg( "rendering v2:\n" + mRendererV2->dump() );

    if ( isEditable() )
    {
      // Destroy all cached geometries and clear the references to them
      deleteCachedGeometries();
      mCachedGeometriesRect = rendererContext.extent();

      // set editing vertex markers style
      mRendererV2->setVertexMarkerAppearance( currentVertexMarkerType(), currentVertexMarkerSize() );
    }

    QgsAttributeList attributes;
    foreach( QString attrName, mRendererV2->usedAttributes() )
    {
      int attrNum = fieldNameIndex( attrName );
      attributes.append( attrNum );
      QgsDebugMsg( "attrs: " + attrName + " - " + QString::number( attrNum ) );
    }

    bool labeling = false;
    if ( rendererContext.labelingEngine() )
    {
      int attrIndex;
      if ( rendererContext.labelingEngine()->prepareLayer( this, attrIndex, rendererContext ) )
      {
        if ( !attributes.contains( attrIndex ) )
          attributes << attrIndex;
        labeling = true;
      }
    }

    QgsFeatureIterator fi = getFeatures( attributes, rendererContext.extent() );

    if ( mRendererV2->usingSymbolLevels() )
      drawRendererV2Levels( rendererContext, labeling, fi );
    else
      drawRendererV2( rendererContext, labeling, fi );

    fi.close();

    return true;
  }

  //draw ( p, viewExtent, theMapToPixelTransform, ct, drawingToEditingCanvas, 1., 1.);

  if ( mRenderer )
  {
    // painter is active (begin has been called
    /* Steps to draw the layer
       1. get the features in the view extent by SQL query
       2. read WKB for a feature
       3. transform
       4. draw
    */

    QPen pen;
    /*Pointer to a marker image*/
    QImage marker;
    //vertex marker type for selection
    QgsVectorLayer::VertexMarkerType vertexMarker = QgsVectorLayer::NoMarker;
    int vertexMarkerSize = 7;

    if ( isEditable() )
    {
      // Destroy all cached geometries and clear the references to them
      deleteCachedGeometries();
      mCachedGeometriesRect = rendererContext.extent();
      vertexMarker = currentVertexMarkerType();
      vertexMarkerSize = currentVertexMarkerSize();
      QSettings settings;
      mVertexMarkerOnlyForSelection = settings.value( "/qgis/digitizing/marker_only_for_selected", false ).toBool();
    }

    // int totalFeatures = pendingFeatureCount();
    int featureCount = 0;
    QgsFeature fet;
    QgsAttributeList attributes = mRenderer->classificationAttributes();

    bool labeling = false;
    if ( rendererContext.labelingEngine() )
    {
      int attrIndex;
      if ( rendererContext.labelingEngine()->prepareLayer( this, attrIndex, rendererContext ) )
      {
        if ( !attributes.contains( attrIndex ) )
          attributes << attrIndex;
        labeling = true;
      }
    }

    QgsFeatureIterator fi = getFeatures( attributes, rendererContext.extent() );

    try
    {
      while ( fi.nextFeature( fet ) )
      {

        if ( rendererContext.renderingStopped() )
        {
          break;
        }

        // check if feature is selected
        // only show selections of the current layer
        // TODO: create a mechanism to let layer know whether it's current layer or not [MD]
        bool sel = mSelectedFeatureIds.contains( fet.id() );

        mCurrentVertexMarkerType = QgsVectorLayer::NoMarker;
        mCurrentVertexMarkerSize = 7;

        if ( isEditable() )
        {
          // Cache this for the use of (e.g.) modifying the feature's uncommitted geometry.
          mCachedGeometries[fet.id()] = *fet.geometry();

          if ( !mVertexMarkerOnlyForSelection || sel )
          {
            mCurrentVertexMarkerType = vertexMarker;
            mCurrentVertexMarkerSize = vertexMarkerSize;
          }
        }

        //QgsDebugMsg(QString("markerScale before renderFeature(): %1").arg(markerScaleFactor));
        // markerScalerFactore reflects the wanted scaling of the marker

        double opacity = 1.0;
        if ( !mRenderer->usesTransparency() )
        {
          opacity = ( mTransparencyLevel * 1.0 ) / 255.0;
        }
        mRenderer->renderFeature( rendererContext, fet, &marker, sel, opacity );

        // markerScalerFactore now reflects the actual scaling of the marker that the render performed.
        //QgsDebugMsg(QString("markerScale after renderFeature(): %1").arg(markerScaleFactor));

        //double scale = rendererContext.scaleFactor() /  markerScaleFactor;
        drawFeature( rendererContext, fet, &marker );

        if ( labeling && mRenderer->willRenderFeature( &fet ) )
        {
          rendererContext.labelingEngine()->registerFeature( this, fet, rendererContext );
        }

        ++featureCount;
      }
    }
    catch ( QgsCsException &cse )
    {
      QgsDebugMsg( QString( "Failed to transform a point while drawing a feature of type '%1'. Rendering stopped. %2" )
                   .arg( fet.typeName() ).arg( cse.what() ) );
      return false;
    }
  }
  else
  {
    QgsDebugMsg( "QgsRenderer is null" );
  }

  if ( isEditable() )
  {
    QgsDebugMsg( QString( "Cached %1 geometries." ).arg( mCachedGeometries.count() ) );
  }

  return true; // Assume success always
}

void QgsVectorLayer::deleteCachedGeometries()
{
  // Destroy any cached geometries
  mCachedGeometries.clear();
  mCachedGeometriesRect = QgsRectangle();
}

void QgsVectorLayer::drawVertexMarker( double x, double y, QPainter& p, QgsVectorLayer::VertexMarkerType type, int m )
{
  if ( type == QgsVectorLayer::SemiTransparentCircle )
  {
    p.setPen( QColor( 50, 100, 120, 200 ) );
    p.setBrush( QColor( 200, 200, 210, 120 ) );
    p.drawEllipse( x - m, y - m, m*2 + 1, m*2 + 1 );
  }
  else if ( type == QgsVectorLayer::Cross )
  {
    p.setPen( QColor( 255, 0, 0 ) );
    p.drawLine( x - m, y + m, x + m, y - m );
    p.drawLine( x - m, y - m, x + m, y + m );
  }
}

void QgsVectorLayer::select( int number, bool emitSignal )
{
  mSelectedFeatureIds.insert( number );

  if ( emitSignal )
  {
    emit selectionChanged();
  }
}

void QgsVectorLayer::deselect( int number, bool emitSignal )
{
  mSelectedFeatureIds.remove( number );

  if ( emitSignal )
  {
    emit selectionChanged();
  }
}

void QgsVectorLayer::select( QgsRectangle & rect, bool lock )
{
  // normalize the rectangle
  rect.normalize();

  if ( !lock )
  {
    removeSelection( false ); // don't emit signal
  }

  //select all the elements
  select( QgsAttributeList(), rect, false, true );

  QgsFeature f;
  while ( nextFeature( f ) )
  {
    select( f.id(), false ); // don't emit signal (not to redraw it everytime)
  }

  emit selectionChanged(); // now emit signal to redraw layer
}

void QgsVectorLayer::invertSelection()
{
  // copy the ids of selected features to tmp
  QgsFeatureIds tmp = mSelectedFeatureIds;

  removeSelection( false ); // don't emit signal

  select( QgsAttributeList(), QgsRectangle(), false );

  QgsFeature fet;
  while ( nextFeature( fet ) )
  {
    select( fet.id(), false ); // don't emit signal
  }

  for ( QgsFeatureIds::iterator iter = tmp.begin(); iter != tmp.end(); ++iter )
  {
    mSelectedFeatureIds.remove( *iter );
  }

  emit selectionChanged();
}

void QgsVectorLayer::invertSelectionInRectangle( QgsRectangle & rect )
{
  // normalize the rectangle
  rect.normalize();

  select( QgsAttributeList(), rect, false, true );

  QgsFeature fet;
  while ( nextFeature( fet ) )
  {
    if ( mSelectedFeatureIds.contains( fet.id() ) )
    {
      deselect( fet.id(), false ); // don't emit signal
    }
    else
    {
      select( fet.id(), false ); // don't emit signal
    }
  }

  emit selectionChanged();
}

void QgsVectorLayer::removeSelection( bool emitSignal )
{
  if ( mSelectedFeatureIds.size() == 0 )
    return;

  mSelectedFeatureIds.clear();

  if ( emitSignal )
  {
    emit selectionChanged();
  }
}

void QgsVectorLayer::triggerRepaint()
{
  emit dataChanged(); // invalidates caches of map renderers
  emit repaintRequested();
}

QgsVectorDataProvider* QgsVectorLayer::dataProvider()
{
  return mDataProvider;
}

const QgsVectorDataProvider* QgsVectorLayer::dataProvider() const
{
  return mDataProvider;
}

void QgsVectorLayer::setProviderEncoding( const QString& encoding )
{
  if ( mDataProvider )
  {
    mDataProvider->setEncoding( encoding );
  }
}


const QgsRenderer* QgsVectorLayer::renderer() const
{
  return mRenderer;
}

void QgsVectorLayer::setRenderer( QgsRenderer * r )
{
  if ( r != mRenderer )
  {
    delete mRenderer;
    mRenderer = r;
  }
}

QGis::GeometryType QgsVectorLayer::geometryType() const
{
  if ( mDataProvider )
  {
    int type = mDataProvider->geometryType();
    switch ( type )
    {
      case QGis::WKBPoint:
      case QGis::WKBPoint25D:
        return QGis::Point;

      case QGis::WKBLineString:
      case QGis::WKBLineString25D:
        return QGis::Line;

      case QGis::WKBPolygon:
      case QGis::WKBPolygon25D:
        return QGis::Polygon;

      case QGis::WKBMultiPoint:
      case QGis::WKBMultiPoint25D:
        return QGis::Point;

      case QGis::WKBMultiLineString:
      case QGis::WKBMultiLineString25D:
        return QGis::Line;

      case QGis::WKBMultiPolygon:
      case QGis::WKBMultiPolygon25D:
        return QGis::Polygon;
    }
    QgsDebugMsg( QString( "Data Provider Geometry type is not recognised, is %1" ).arg( type ) );
  }
  else
  {
    QgsDebugMsg( "pointer to mDataProvider is null" );
  }

  // We shouldn't get here, and if we have, other things are likely to
  // go wrong. Code that uses the type() return value should be
  // rewritten to cope with a value of QGis::Unknown. To make this
  // need known, the following message is printed every time we get
  // here.
  QgsDebugMsg( "WARNING: This code should never be reached. Problems may occur..." );

  return QGis::UnknownGeometry;
}

QGis::WkbType QgsVectorLayer::wkbType() const
{
  return ( QGis::WkbType )( mWkbType );
}

QgsRectangle QgsVectorLayer::boundingBoxOfSelected()
{
  if ( mSelectedFeatureIds.size() == 0 )//no selected features
  {
    return QgsRectangle( 0, 0, 0, 0 );
  }

  QgsRectangle r, retval;


  select( QgsAttributeList(), QgsRectangle(), true );

  retval.setMinimal();

  QgsFeature fet;
  while ( nextFeature( fet ) )
  {
    if ( mSelectedFeatureIds.contains( fet.id() ) )
    {
      if ( fet.geometry() )
      {
        r = fet.geometry()->boundingBox();
        retval.combineExtentWith( &r );
      }
    }
  }

  if ( retval.width() == 0.0 || retval.height() == 0.0 )
  {
    // If all of the features are at the one point, buffer the
    // rectangle a bit. If they are all at zero, do something a bit
    // more crude.

    if ( retval.xMinimum() == 0.0 && retval.xMaximum() == 0.0 &&
         retval.yMinimum() == 0.0 && retval.yMaximum() == 0.0 )
    {
      retval.set( -1.0, -1.0, 1.0, 1.0 );
    }
  }

  return retval;
}



long QgsVectorLayer::featureCount() const
{
  if ( !mDataProvider )
  {
    QgsDebugMsg( "invoked with null mDataProvider" );
    return 0;
  }

  return mDataProvider->featureCount();
}

long QgsVectorLayer::updateFeatureCount() const
{
  return -1;
}

void QgsVectorLayer::updateExtents()
{
  if ( !mDataProvider )
    QgsDebugMsg( "invoked with null mDataProvider" );

  if (mEditBuffer)
    mLayerExtent = mEditBuffer->layerExtent();
  else
    mLayerExtent = mDataProvider->extent();

  if ( mLayerExtent.xMinimum() > mLayerExtent.xMaximum() && mLayerExtent.yMinimum() > mLayerExtent.yMaximum() )
  {
    // special case when there are no features in provider nor any added
    mLayerExtent = QgsRectangle(); // use rectangle with zero coordinates
  }

  // Send this (hopefully) up the chain to the map canvas
  emit recalculateExtents();
}

QString QgsVectorLayer::subsetString()
{
  if ( ! mDataProvider )
  {
    QgsDebugMsg( "invoked with null mDataProvider" );
    return 0;
  }
  return mDataProvider->subsetString();
}

bool QgsVectorLayer::setSubsetString( QString subset )
{
  if ( ! mDataProvider )
  {
    QgsDebugMsg( "invoked with null mDataProvider" );
    return false;
  }

  bool res = mDataProvider->setSubsetString( subset );

  // get the updated data source string from the provider
  mDataSource = mDataProvider->dataSourceUri();
  updateExtents();

  if ( res )
    emit dataChanged();

  return res;
}


void QgsVectorLayer::select( QgsAttributeList attributes, QgsRectangle rect, bool fetchGeometries, bool useIntersect )
{
  if (!mOldApiIter.isClosed())
    mOldApiIter.close();
  mOldApiIter = getFeatures( attributes, rect, fetchGeometries, useIntersect );
}

bool QgsVectorLayer::nextFeature( QgsFeature &f )
{
  return mOldApiIter.nextFeature(f);
}

QgsFeatureIterator QgsVectorLayer::getFeatures( QgsAttributeList fetchAttributes,
                                                QgsRectangle rect,
                                                bool fetchGeometry,
                                                bool useIntersect )
{
  return QgsFeatureIterator( new QgsVectorLayerIterator( this, fetchAttributes, rect, fetchGeometry, useIntersect ) );
}


bool QgsVectorLayer::featureAtId( int featureId, QgsFeature& f, bool fetchGeometries, bool fetchAttributes )
{
  if ( !mDataProvider )
    return false;

  if (mEditBuffer)
    return mEditBuffer->featureAtId(featureId, f, fetchGeometries, fetchAttributes);
  else
    return mDataProvider->featureAtId(featureId, f, fetchGeometries, pendingAllAttributesList());
}

bool QgsVectorLayer::addFeature( QgsFeature& f, bool alsoUpdateExtent )
{
  if (mEditBuffer)
    return mEditBuffer->addFeature(f, alsoUpdateExtent);
  else
    return false;
}


bool QgsVectorLayer::insertVertex( double x, double y, int atFeatureId, int beforeVertex )
{
  if (mEditBuffer)
    return mEditBuffer->insertVertex(x,y, atFeatureId, beforeVertex);
  else
    return false;
}


bool QgsVectorLayer::moveVertex( double x, double y, int atFeatureId, int atVertex )
{
  if (mEditBuffer)
    return mEditBuffer->moveVertex(x,y, atFeatureId, atVertex);
  else
    return false;
}


bool QgsVectorLayer::deleteVertex( int atFeatureId, int atVertex )
{
  if (mEditBuffer)
    return mEditBuffer->deleteVertex(atFeatureId, atVertex);
  else
    return false;
}


bool QgsVectorLayer::deleteSelectedFeatures()
{
  if ( !( mDataProvider->capabilities() & QgsVectorDataProvider::DeleteFeatures ) )
  {
    return false;
  }

  if ( !isEditable() )
  {
    return false;
  }

  if ( mSelectedFeatureIds.size() == 0 )
    return true;

  while ( mSelectedFeatureIds.size() > 0 )
  {
    int fid = *mSelectedFeatureIds.begin();
    deleteFeature( fid );  // removes from selection
  }

  emit selectionChanged();

  triggerRepaint();
  updateExtents();

  return true;
}

int QgsVectorLayer::addRing( const QList<QgsPoint>& ring )
{
  if (mEditBuffer)
    return mEditBuffer->addRing(ring);
  else
    return -1;
}

int QgsVectorLayer::addIsland( const QList<QgsPoint>& ring )
{
  if (mEditBuffer)
    return mEditBuffer->addIsland(ring);
  else
    return -1;
}

int QgsVectorLayer::translateFeature( int featureId, double dx, double dy )
{
  if (mEditBuffer)
    return mEditBuffer->translateFeature(featureId, dx, dy);
  else
    return -1;
}

int QgsVectorLayer::splitFeatures( const QList<QgsPoint>& splitLine, bool topologicalEditing )
{
  if (mEditBuffer)
    return mEditBuffer->splitFeatures(splitLine, topologicalEditing);
  else
    return -1;
}

int QgsVectorLayer::removePolygonIntersections( QgsGeometry* geom )
{
  int returnValue = 0;

  //first test if geom really has type polygon or multipolygon
  if ( geom->type() != QGis::Polygon )
  {
    return 1;
  }

  //get bounding box of geom
  QgsRectangle geomBBox = geom->boundingBox();

  //get list of features that intersect this bounding box
  select( QgsAttributeList(), geomBBox, true, true );

  QgsFeature f;
  while ( nextFeature( f ) )
  {
    //call geometry->makeDifference for each feature
    QgsGeometry *currentGeom = f.geometry();
    if ( currentGeom )
    {
      if ( geom->makeDifference( currentGeom ) != 0 )
      {
        returnValue = 2;
      }
    }
  }
  return returnValue;
}

QgsLabel * QgsVectorLayer::label()
{
  return mLabel;
}

const QgsLabel *QgsVectorLayer::label() const
{
  return mLabel;
}

void QgsVectorLayer::enableLabels( bool on )
{
  mLabelOn = on;
}

bool QgsVectorLayer::hasLabelsEnabled( void ) const
{
  return mLabelOn;
}

bool QgsVectorLayer::startEditing()
{
  if ( !mDataProvider )
  {
    return false;
  }

  // allow editing if provider supports any of the capabilities
  if ( !( mDataProvider->capabilities() & QgsVectorDataProvider::EditingCapabilities ) )
  {
    return false;
  }

  if ( isEditable() )
  {
    // editing already underway
    return false;
  }

  mEditBuffer = new QgsVectorLayerEditBuffer(this);

  emit editingStarted();

  return true;
}

bool QgsVectorLayer::readXml( QDomNode & layer_node )
{
  QgsDebugMsg( QString( "Datasource in QgsVectorLayer::readXml: " ) + mDataSource.toLocal8Bit().data() );

  //process provider key
  QDomNode pkeyNode = layer_node.namedItem( "provider" );

  if ( pkeyNode.isNull() )
  {
    mProviderKey = "";
  }
  else
  {
    QDomElement pkeyElt = pkeyNode.toElement();
    mProviderKey = pkeyElt.text();
  }

  // determine type of vector layer
  if ( ! mProviderKey.isNull() )
  {
    // if the provider string isn't empty, then we successfully
    // got the stored provider
  }
  else if ( mDataSource.contains( "dbname=" ) )
  {
    mProviderKey = "postgres";
  }
  else
  {
    mProviderKey = "ogr";
  }

  if ( ! setDataProvider( mProviderKey ) )
  {
    return false;
  }

  QDomElement pkeyElem = pkeyNode.toElement();
  if ( !pkeyElem.isNull() )
  {
    QString encodingString = pkeyElem.attribute( "encoding" );
    if ( !encodingString.isEmpty() )
    {
      mDataProvider->setEncoding( encodingString );
    }
  }

  QString errorMsg;
  if ( !readSymbology( layer_node, errorMsg ) )
  {
    return false;
  }

  return mValid;               // should be true if read successfully

} // void QgsVectorLayer::readXml



bool QgsVectorLayer::setDataProvider( QString const & provider )
{
  // XXX should I check for and possibly delete any pre-existing providers?
  // XXX How often will that scenario occur?

  mProviderKey = provider;     // XXX is this necessary?  Usually already set
  // XXX when execution gets here.

  //XXX - This was a dynamic cast but that kills the Windows
  //      version big-time with an abnormal termination error
  mDataProvider =
    ( QgsVectorDataProvider* )( QgsProviderRegistry::instance()->getProvider( provider, mDataSource ) );

  if ( mDataProvider )
  {
    QgsDebugMsg( "Instantiated the data provider plugin" );

    mValid = mDataProvider->isValid();
    if ( mValid )
    {

      // TODO: Check if the provider has the capability to send fullExtentCalculated
      connect( mDataProvider, SIGNAL( fullExtentCalculated() ), this, SLOT( updateExtents() ) );

      connect( mDataProvider, SIGNAL(dataChanged()), this, SIGNAL(dataChanged()) );

      // get the extent
      QgsRectangle mbr = mDataProvider->extent();

      // show the extent
      QString s = mbr.toString();
      QgsDebugMsg( "Extent of layer: " +  s );
      // store the extent
      mLayerExtent.setXMaximum( mbr.xMaximum() );
      mLayerExtent.setXMinimum( mbr.xMinimum() );
      mLayerExtent.setYMaximum( mbr.yMaximum() );
      mLayerExtent.setYMinimum( mbr.yMinimum() );

      // get and store the feature type
      mWkbType = mDataProvider->geometryType();

      // look at the fields in the layer and set the primary
      // display field using some real fuzzy logic
      setDisplayField();

      if ( mProviderKey == "postgres" )
      {
        QgsDebugMsg( "Beautifying layer name " + name() );

        // adjust the display name for postgres layers
        QRegExp reg( "\"[^\"]+\"\\.\"([^\"]+)\" \\(([^)]+)\\)" );
        if ( reg.indexIn( name() ) >= 0 )
        {
          QStringList stuff = reg.capturedTexts();
          QString lName = stuff[1];

          const QMap<QString, QgsMapLayer*> &layers = QgsMapLayerRegistry::instance()->mapLayers();

          QMap<QString, QgsMapLayer*>::const_iterator it;
          for ( it = layers.constBegin(); it != layers.constEnd() && ( *it )->name() != lName; it++ )
            ;

          if ( it != layers.constEnd() )
            lName += "." + stuff[2];

          if ( !lName.isEmpty() )
            setLayerName( lName );
        }

        QgsDebugMsg( "Beautifying layer name " + name() );

        // deal with unnecessary schema qualification to make v.in.ogr happy
        mDataSource = mDataProvider->dataSourceUri();
      }
      else if ( mProviderKey == "osm" )
      {
        // make sure that the "observer" has been removed from URI to avoid crashes
        mDataSource = mDataProvider->dataSourceUri();
      }

      // label
      mLabel = new QgsLabel( mDataProvider->fields() );
      mLabelOn = false;
    }
    else
    {
      QgsDebugMsg( "Invalid provider plugin " + QString( mDataSource.toUtf8() ) );
      return false;
    }
  }
  else
  {
    QgsDebugMsg( " unable to get data provider" );

    return false;
  }

  return true;

} // QgsVectorLayer:: setDataProvider




/* virtual */
bool QgsVectorLayer::writeXml( QDomNode & layer_node,
                               QDomDocument & document )
{
  // first get the layer element so that we can append the type attribute

  QDomElement mapLayerNode = layer_node.toElement();

  if ( mapLayerNode.isNull() || ( "maplayer" != mapLayerNode.nodeName() ) )
  {
    QgsDebugMsg( "can't find <maplayer>" );
    return false;
  }

  mapLayerNode.setAttribute( "type", "vector" );

  // set the geometry type
  mapLayerNode.setAttribute( "geometry", QGis::qgisVectorGeometryType[geometryType()] );

  // add provider node
  if ( mDataProvider )
  {
    QDomElement provider  = document.createElement( "provider" );
    provider.setAttribute( "encoding", mDataProvider->encoding() );
    QDomText providerText = document.createTextNode( providerType() );
    provider.appendChild( providerText );
    layer_node.appendChild( provider );
  }

  // renderer specific settings
  QString errorMsg;
  if ( !writeSymbology( layer_node, document, errorMsg ) )
  {
    return false;
  }

  return true;
} // bool QgsVectorLayer::writeXml

bool QgsVectorLayer::readSymbology( const QDomNode& node, QString& errorMessage )
{
  // try renderer v2 first
  QDomElement rendererElement = node.firstChildElement( RENDERER_TAG_NAME );
  if ( !rendererElement.isNull() )
  {
    // using renderer v2
    setUsingRendererV2( true );

    QgsFeatureRendererV2* r = QgsFeatureRendererV2::load( rendererElement );
    if ( r == NULL )
      return false;

    setRendererV2( r );
  }
  else
  {
    // using renderer v1
    setUsingRendererV2( false );

    // create and bind a renderer to this layer

    QDomNode singlenode = node.namedItem( "singlesymbol" );
    QDomNode graduatednode = node.namedItem( "graduatedsymbol" );
    QDomNode continuousnode = node.namedItem( "continuoussymbol" );
    QDomNode uniquevaluenode = node.namedItem( "uniquevalue" );

    QgsRenderer * renderer = 0;
    int returnCode = 1;

    if ( !singlenode.isNull() )
    {
      renderer = new QgsSingleSymbolRenderer( geometryType() );
      returnCode = renderer->readXML( singlenode, *this );
    }
    else if ( !graduatednode.isNull() )
    {
      renderer = new QgsGraduatedSymbolRenderer( geometryType() );
      returnCode = renderer->readXML( graduatednode, *this );
    }
    else if ( !continuousnode.isNull() )
    {
      renderer = new QgsContinuousColorRenderer( geometryType() );
      returnCode = renderer->readXML( continuousnode, *this );
    }
    else if ( !uniquevaluenode.isNull() )
    {
      renderer = new QgsUniqueValueRenderer( geometryType() );
      returnCode = renderer->readXML( uniquevaluenode, *this );
    }

    if ( !renderer )
    {
      errorMessage = tr( "Unknown renderer" );
      return false;
    }

    if ( returnCode == 1 )
    {
      errorMessage = tr( "No renderer object" ); delete renderer; return false;
    }
    else if ( returnCode == 2 )
    {
      errorMessage = tr( "Classification field not found" ); delete renderer; return false;
    }

    mRenderer = renderer;

  }

  // process the attribute actions
  mActions->readXML( node );

  // get and set the display field if it exists.
  QDomNode displayFieldNode = node.namedItem( "displayfield" );
  if ( !displayFieldNode.isNull() )
  {
    QDomElement e = displayFieldNode.toElement();
    setDisplayField( e.text() );
  }

  // use scale dependent visibility flag
  QDomElement e = node.toElement();
  label()->setScaleBasedVisibility( e.attribute( "scaleBasedLabelVisibilityFlag", "0" ) == "1" );
  label()->setMinScale( e.attribute( "minLabelScale", "1" ).toFloat() );
  label()->setMaxScale( e.attribute( "maxLabelScale", "100000000" ).toFloat() );

  mEditTypes.clear();
  QDomNode editTypesNode = node.namedItem( "edittypes" );
  if ( !editTypesNode.isNull() )
  {
    QDomNodeList editTypeNodes = editTypesNode.childNodes();

    for ( int i = 0; i < editTypeNodes.size(); i++ )
    {
      QDomNode editTypeNode = editTypeNodes.at( i );
      QDomElement editTypeElement = editTypeNode.toElement();

      QString name = editTypeElement.attribute( "name" );

      EditType editType = ( EditType ) editTypeElement.attribute( "type" ).toInt();
      mEditTypes.insert( name, editType );

      if ( editType == ValueMap && editTypeNode.hasChildNodes() )
      {
        mValueMaps.insert( name, QMap<QString, QVariant>() );

        QDomNodeList valueMapNodes = editTypeNode.childNodes();
        for ( int j = 0; j < valueMapNodes.size(); j++ )
        {
          QDomElement value = valueMapNodes.at( j ).toElement();
          mValueMaps[ name ].insert( value.attribute( "key" ), value.attribute( "value" ) );
        }
      }
      else if ( editType == EditRange || editType == SliderRange )
      {
        QVariant min = editTypeElement.attribute( "min" );
        QVariant max = editTypeElement.attribute( "max" );
        QVariant step = editTypeElement.attribute( "step" );

        mRanges[ name ] = RangeData( min, max, step );
      }
      else if ( editType == CheckBox )
      {
        mCheckedStates[ name ] = QPair<QString, QString>( editTypeElement.attribute( "checked" ), editTypeElement.attribute( "unchecked" ) );
      }
    }
  }

  QDomNode editFormNode = node.namedItem( "editform" );
  if ( !editFormNode.isNull() )
  {
    QDomElement e = editFormNode.toElement();
    mEditForm = QgsProject::instance()->readPath( e.text() );
  }

  QDomNode editFormInitNode = node.namedItem( "editforminit" );
  if ( !editFormInitNode.isNull() )
  {
    mEditFormInit = editFormInitNode.toElement().text();
  }

  QDomNode annotationFormNode = node.namedItem( "annotationform" );
  if ( !annotationFormNode.isNull() )
  {
    QDomElement e = annotationFormNode.toElement();
    mAnnotationForm = QgsProject::instance()->readPath( e.text() );
  }

  mAttributeAliasMap.clear();
  QDomNode aliasesNode = node.namedItem( "aliases" );
  if ( !aliasesNode.isNull() )
  {
    QDomElement aliasElem;
    int index;
    QString name;

    QDomNodeList aliasNodeList = aliasesNode.toElement().elementsByTagName( "alias" );
    for ( int i = 0; i < aliasNodeList.size(); ++i )
    {
      aliasElem = aliasNodeList.at( i ).toElement();
      index = aliasElem.attribute( "index" ).toInt();
      name = aliasElem.attribute( "name" );
      mAttributeAliasMap.insert( index, name );
    }
  }

  // Test if labeling is on or off
  QDomNode labelnode = node.namedItem( "label" );
  QDomElement element = labelnode.toElement();
  int hasLabelsEnabled = element.text().toInt();
  if ( hasLabelsEnabled < 1 )
  {
    enableLabels( false );
  }
  else
  {
    enableLabels( true );
  }

  QDomNode labelattributesnode = node.namedItem( "labelattributes" );

  if ( !labelattributesnode.isNull() )
  {
    QgsDebugMsg( "qgsvectorlayer calling label readXML routine" );
    mLabel->readXML( labelattributesnode );
  }

  return true;
}

bool QgsVectorLayer::writeSymbology( QDomNode& node, QDomDocument& doc, QString& errorMessage ) const
{
  if ( mUsingRendererV2 )
  {
    QDomElement rendererElement = mRendererV2->save( doc );
    node.appendChild( rendererElement );
  }
  else
  {
    //classification field(s)
    QgsAttributeList attributes = mRenderer->classificationAttributes();
    const QgsFieldMap providerFields = mDataProvider->fields();
    for ( QgsAttributeList::const_iterator it = attributes.begin(); it != attributes.end(); ++it )
    {
      QDomElement classificationElement = doc.createElement( "classificationattribute" );
      QDomText classificationText = doc.createTextNode( providerFields[*it].name() );
      classificationElement.appendChild( classificationText );
      node.appendChild( classificationElement );
    }

    // renderer settings
    const QgsRenderer * myRenderer = renderer();
    if ( myRenderer )
    {
      if ( !myRenderer->writeXML( node, doc, *this ) )
      {
        errorMessage = "renderer failed to save";
        return false;
      }
    }
    else
    {
      QgsDebugMsg( "no renderer" );
      errorMessage = "no renderer";
      return false;
    }
  }

  // use scale dependent visibility flag
  QDomElement mapLayerNode = node.toElement();
  mapLayerNode.setAttribute( "scaleBasedLabelVisibilityFlag", label()->scaleBasedVisibility() ? 1 : 0 );
  mapLayerNode.setAttribute( "minLabelScale", label()->minScale() );
  mapLayerNode.setAttribute( "maxLabelScale", label()->maxScale() );

  //edit types
  if ( mEditTypes.size() > 0 )
  {
    QDomElement editTypesElement = doc.createElement( "edittypes" );

    for ( QMap<QString, EditType>::const_iterator it = mEditTypes.begin(); it != mEditTypes.end(); ++it )
    {
      QDomElement editTypeElement = doc.createElement( "edittype" );
      editTypeElement.setAttribute( "name", it.key() );
      editTypeElement.setAttribute( "type", it.value() );

      if ( it.value() == ValueMap )
      {
        if ( mValueMaps.contains( it.key() ) )
        {
          const QMap<QString, QVariant> &map = mValueMaps[ it.key()];

          for ( QMap<QString, QVariant>::const_iterator vmit = map.begin(); vmit != map.end(); vmit++ )
          {
            QDomElement value = doc.createElement( "valuepair" );
            value.setAttribute( "key", vmit.key() );
            value.setAttribute( "value", vmit.value().toString() );
            editTypeElement.appendChild( value );
          }
        }
      }
      else if ( it.value() == EditRange || it.value() == SliderRange )
      {
        if ( mRanges.contains( it.key() ) )
        {
          editTypeElement.setAttribute( "min", mRanges[ it.key()].mMin.toString() );
          editTypeElement.setAttribute( "max", mRanges[ it.key()].mMax.toString() );
          editTypeElement.setAttribute( "step", mRanges[ it.key()].mStep.toString() );
        }
      }
      else if ( it.value() == CheckBox )
      {
        if ( mCheckedStates.contains( it.key() ) )
        {
          editTypeElement.setAttribute( "checked", mCheckedStates[ it.key()].first );
          editTypeElement.setAttribute( "unchecked", mCheckedStates[ it.key()].second );
        }
      }

      editTypesElement.appendChild( editTypeElement );
    }

    node.appendChild( editTypesElement );
  }

  QDomElement efField  = doc.createElement( "editform" );
  QDomText efText = doc.createTextNode( QgsProject::instance()->writePath( mEditForm ) );
  efField.appendChild( efText );
  node.appendChild( efField );

  QDomElement efiField  = doc.createElement( "editforminit" );
  QDomText efiText = doc.createTextNode( mEditFormInit );
  efiField.appendChild( efiText );
  node.appendChild( efiField );

  QDomElement afField = doc.createElement( "annotationform" );
  QDomText afText = doc.createTextNode( QgsProject::instance()->writePath( mAnnotationForm ) );
  afField.appendChild( afText );
  node.appendChild( afField );

  //attribute aliases
  if ( mAttributeAliasMap.size() > 0 )
  {
    QDomElement aliasElem = doc.createElement( "aliases" );
    QMap<int, QString>::const_iterator a_it = mAttributeAliasMap.constBegin();
    for ( ; a_it != mAttributeAliasMap.constEnd(); ++a_it )
    {
      QDomElement aliasEntryElem = doc.createElement( "alias" );
      aliasEntryElem.setAttribute( "index", QString::number( a_it.key() ) );
      aliasEntryElem.setAttribute( "name", a_it.value() );
      aliasElem.appendChild( aliasEntryElem );
    }
    node.appendChild( aliasElem );
  }

  // add the display field
  QDomElement dField  = doc.createElement( "displayfield" );
  QDomText dFieldText = doc.createTextNode( displayField() );
  dField.appendChild( dFieldText );
  node.appendChild( dField );

  // add label node
  QDomElement labelElem = doc.createElement( "label" );
  QDomText labelText = doc.createTextNode( "" );

  if ( hasLabelsEnabled() )
  {
    labelText.setData( "1" );
  }
  else
  {
    labelText.setData( "0" );
  }
  labelElem.appendChild( labelText );

  node.appendChild( labelElem );

  // add attribute actions
  mActions->writeXML( node, doc );

  // Now we get to do all that all over again for QgsLabel

  // XXX Since this is largely a cut-n-paste from the previous, this
  // XXX therefore becomes a candidate to be generalized into a separate
  // XXX function.  I think.

  const QgsLabel *myLabel = label();

  if ( myLabel )
  {
    QString fieldname = myLabel->labelField( QgsLabel::Text );
    if ( fieldname != "" )
    {
      dField  = doc.createElement( "labelfield" );
      dFieldText = doc.createTextNode( fieldname );
      dField.appendChild( dFieldText );
      node.appendChild( dField );
    }

    myLabel->writeXML( node, doc );
  }

  //save vector overlays (e.g. diagrams)
  QList<QgsVectorOverlay*>::const_iterator overlay_it = mOverlays.constBegin();
  for ( ; overlay_it != mOverlays.constEnd(); ++overlay_it )
  {
    if ( *overlay_it )
    {
      ( *overlay_it )->writeXML( mapLayerNode, doc );
    }
  }

  return true;
}


bool QgsVectorLayer::changeGeometry( int fid, QgsGeometry* geom )
{
  if (mEditBuffer)
    mEditBuffer->changeGeometry(fid, geom);
  else
    return false;
}


bool QgsVectorLayer::changeAttributeValue( int fid, int field, QVariant value, bool emitSignal )
{
  if (mEditBuffer)
    mEditBuffer->changeAttributeValue(fid, field, value, emitSignal);
  else
    return false;
}

bool QgsVectorLayer::addAttribute( const QgsField &field )
{
  if (mEditBuffer)
    mEditBuffer->addAttribute(field);
  else
    return false;
}

bool QgsVectorLayer::addAttribute( QString name, QString type )
{
  const QMap<QString, QVariant::Type> &map = mDataProvider->supportedNativeTypes();

  if ( !map.contains( type ) )
    return false;

  return addAttribute( QgsField( name, map[ type ], type ) );
}

void QgsVectorLayer::addAttributeAlias( int attIndex, QString aliasString )
{
  mAttributeAliasMap.insert( attIndex, aliasString );
  emit layerModified( false );
}

QString QgsVectorLayer::attributeAlias( int attributeIndex ) const
{
  QMap<int, QString>::const_iterator alias_it = mAttributeAliasMap.find( attributeIndex );
  if ( alias_it != mAttributeAliasMap.constEnd() )
  {
    return alias_it.value();
  }
  else
  {
    return QString();
  }
}

QString QgsVectorLayer::attributeDisplayName( int attributeIndex ) const
{
  QString displayName = attributeAlias( attributeIndex );
  if ( displayName.isEmpty() )
  {
    const QgsFieldMap& fields = pendingFields();
    QgsFieldMap::const_iterator fieldIt = fields.find( attributeIndex );
    if ( fieldIt != fields.constEnd() )
    {
      displayName = fieldIt->name();
    }
  }
  return displayName;
}

bool QgsVectorLayer::deleteAttribute( int index )
{
  if (mEditBuffer)
    mEditBuffer->deleteAttribute(index);
  else
    return false;
}

bool QgsVectorLayer::deleteFeature( int fid )
{
  if (mEditBuffer)
    mEditBuffer->deleteFeature(fid);
  else
    return false;
}

const QgsFieldMap &QgsVectorLayer::pendingFields() const
{
  if (mEditBuffer)
    return mEditBuffer->pendingFields();
  else
    return mDataProvider->fields();
}

QgsAttributeList QgsVectorLayer::pendingAllAttributesList()
{
  if (mEditBuffer)
    return mEditBuffer->pendingAllAttributesList();
  else
    return mDataProvider->attributeIndexes();
}

int QgsVectorLayer::pendingFeatureCount()
{
  if (mEditBuffer)
    return mEditBuffer->pendingFeatureCount();
  else
    return mDataProvider->featureCount();
}

bool QgsVectorLayer::commitChanges()
{
  mCommitErrors.clear();

  if ( !mDataProvider )
  {
    mCommitErrors << tr( "ERROR: no provider" );
    return false;
  }

  if ( mEditBuffer )
  {
    bool success = mEditBuffer->commitChanges(mCommitErrors);

    if ( success )
    {
      delete mEditBuffer;
      mEditBuffer = NULL;
    }

    return success;
  }
  else
  {
    mCommitErrors << tr( "ERROR: layer not editable" );
    return false;
  }
}

const QStringList &QgsVectorLayer::commitErrors()
{
  return mCommitErrors;
}

bool QgsVectorLayer::rollBack()
{
  if (mEditBuffer)
  {
    return mEditBuffer->rollBack();

    delete mEditBuffer;
    mEditBuffer = NULL;
  }
  else
    return false;
}

void QgsVectorLayer::setSelectedFeatures( const QgsFeatureIds& ids )
{
  // TODO: check whether features with these ID exist
  mSelectedFeatureIds = ids;

  emit selectionChanged();
}

int QgsVectorLayer::selectedFeatureCount()
{
  return mSelectedFeatureIds.size();
}

const QgsFeatureIds& QgsVectorLayer::selectedFeaturesIds() const
{
  return mSelectedFeatureIds;
}


QgsFeatureList QgsVectorLayer::selectedFeatures()
{
  if ( !mDataProvider )
  {
    return QgsFeatureList();
  }

  QgsFeatureList features;
  QgsFeature feat;
  QgsAttributeList pendingAllAttrs = pendingAllAttributesList();

  for ( QgsFeatureIds::iterator it = mSelectedFeatureIds.begin(); it != mSelectedFeatureIds.end(); ++it )
  {
    featureAtId( *it, feat, true, true );

    features << feat;
  } // for each selected

  return features;
}

bool QgsVectorLayer::addFeatures( QgsFeatureList features, bool makeSelected )
{
  if (mEditBuffer)
  {
    mEditBuffer->addFeatures(features);

    if ( makeSelected )
    {
      mSelectedFeatureIds.clear();

      for ( QgsFeatureList::iterator iter = features.begin(); iter != features.end(); ++iter )
        mSelectedFeatureIds.insert( iter->id() );

      emit selectionChanged();
    }
  }
  else
    return false;
}


bool QgsVectorLayer::copySymbologySettings( const QgsMapLayer& other )
{
  const QgsVectorLayer* vl = qobject_cast<const QgsVectorLayer *>( &other );

  // exit if both vectorlayer are the same
  if ( this == vl )
  {
    return false;
  }

  if ( !vl )
  {
    return false;
  }
  delete mRenderer;

  QgsRenderer* r = vl->mRenderer;
  if ( r )
  {
    mRenderer = r->clone();
    return true;
  }
  else
  {
    return false;
  }
}

bool QgsVectorLayer::hasCompatibleSymbology( const QgsMapLayer& other ) const
{
  // vector layers are symbology compatible if they have the same type, the same sequence of numerical/ non numerical fields and the same field names

  const QgsVectorLayer* otherVectorLayer = qobject_cast<const QgsVectorLayer *>( &other );
  if ( otherVectorLayer )
  {
    if ( otherVectorLayer->type() != type() )
    {
      return false;
    }

    const QgsFieldMap& fieldsThis = mDataProvider->fields();
    const QgsFieldMap& fieldsOther = otherVectorLayer ->mDataProvider->fields();

    if ( fieldsThis.size() != fieldsOther.size() )
    {
      return false;
    }

    // TODO: fill two sets with the numerical types for both layers

    uint fieldsThisSize = fieldsThis.size();

    for ( uint i = 0; i < fieldsThisSize; ++i )
    {
      if ( fieldsThis[i].name() != fieldsOther[i].name() ) // field names need to be the same
      {
        return false;
      }
      // TODO: compare types of the fields
    }
    return true; // layers are symbology compatible if the code reaches this point
  }
  return false;
}

bool QgsVectorLayer::snapPoint( QgsPoint& point, double tolerance )
{
  QMultiMap<double, QgsSnappingResult> snapResults;
  int result = snapWithContext( point, tolerance, snapResults, QgsSnapper::SnapToVertex );

  if ( result != 0 )
  {
    return false;
  }

  if ( snapResults.size() < 1 )
  {
    return false;
  }

  QMultiMap<double, QgsSnappingResult>::const_iterator snap_it = snapResults.constBegin();
  point.setX( snap_it.value().snappedVertex.x() );
  point.setY( snap_it.value().snappedVertex.y() );
  return true;
}


int QgsVectorLayer::snapWithContext( const QgsPoint& startPoint, double snappingTolerance,
                                     QMultiMap<double, QgsSnappingResult>& snappingResults,
                                     QgsSnapper::SnappingType snap_to )
{
  if ( snappingTolerance <= 0 || !mDataProvider )
  {
    return 1;
  }

  QList<QgsFeature> featureList;
  QgsRectangle searchRect( startPoint.x() - snappingTolerance, startPoint.y() - snappingTolerance,
                           startPoint.x() + snappingTolerance, startPoint.y() + snappingTolerance );
  double sqrSnappingTolerance = snappingTolerance * snappingTolerance;

  int n = 0;
  QgsFeature f;

  if ( mCachedGeometriesRect.contains( searchRect ) )
  {
    QgsDebugMsg( "Using cached geometries for snapping." );

    QgsGeometryMap::iterator it = mCachedGeometries.begin();
    for ( ; it != mCachedGeometries.end() ; ++it )
    {
      QgsGeometry* g = &( it.value() );
      if ( g->boundingBox().intersects( searchRect ) )
      {
        snapToGeometry( startPoint, it.key(), g, sqrSnappingTolerance, snappingResults, snap_to );
        ++n;
      }
    }
  }
  else
  {
    // snapping outside cached area

    select( QgsAttributeList(), searchRect, true, true );

    while ( nextFeature( f ) )
    {
      snapToGeometry( startPoint, f.id(), f.geometry(), sqrSnappingTolerance, snappingResults, snap_to );
      ++n;
    }
  }

  return n == 0 ? 2 : 0;
}

void QgsVectorLayer::snapToGeometry( const QgsPoint& startPoint, int featureId, QgsGeometry* geom, double sqrSnappingTolerance,
                                     QMultiMap<double, QgsSnappingResult>& snappingResults, QgsSnapper::SnappingType snap_to ) const
{
  if ( !geom )
  {
    return;
  }

  int atVertex, beforeVertex, afterVertex;
  double sqrDistVertexSnap, sqrDistSegmentSnap;
  QgsPoint snappedPoint;
  QgsSnappingResult snappingResultVertex;
  QgsSnappingResult snappingResultSegment;

  if ( snap_to == QgsSnapper::SnapToVertex || snap_to == QgsSnapper::SnapToVertexAndSegment )
  {
    snappedPoint = geom->closestVertex( startPoint, atVertex, beforeVertex, afterVertex, sqrDistVertexSnap );
    if ( sqrDistVertexSnap < sqrSnappingTolerance )
    {
      snappingResultVertex.snappedVertex = snappedPoint;
      snappingResultVertex.snappedVertexNr = atVertex;
      snappingResultVertex.beforeVertexNr = beforeVertex;
      if ( beforeVertex != -1 ) // make sure the vertex is valid
      {
        snappingResultVertex.beforeVertex = geom->vertexAt( beforeVertex );
      }
      snappingResultVertex.afterVertexNr = afterVertex;
      if ( afterVertex != -1 ) // make sure the vertex is valid
      {
        snappingResultVertex.afterVertex = geom->vertexAt( afterVertex );
      }
      snappingResultVertex.snappedAtGeometry = featureId;
      snappingResultVertex.layer = this;
      snappingResults.insert( sqrt( sqrDistVertexSnap ), snappingResultVertex );
      return;
    }
  }
  if ( snap_to == QgsSnapper::SnapToSegment || snap_to == QgsSnapper::SnapToVertexAndSegment ) // snap to segment
  {
    if ( geometryType() != QGis::Point ) // cannot snap to segment for points/multipoints
    {
      sqrDistSegmentSnap = geom->closestSegmentWithContext( startPoint, snappedPoint, afterVertex );

      if ( sqrDistSegmentSnap < sqrSnappingTolerance )
      {
        snappingResultSegment.snappedVertex = snappedPoint;
        snappingResultSegment.snappedVertexNr = -1;
        snappingResultSegment.beforeVertexNr = afterVertex - 1;
        snappingResultSegment.afterVertexNr = afterVertex;
        snappingResultSegment.snappedAtGeometry = featureId;
        snappingResultSegment.beforeVertex = geom->vertexAt( afterVertex - 1 );
        snappingResultSegment.afterVertex = geom->vertexAt( afterVertex );
        snappingResultSegment.layer = this;
        snappingResults.insert( sqrt( sqrDistSegmentSnap ), snappingResultSegment );
      }
    }
  }

}

int QgsVectorLayer::addTopologicalPoints( QgsGeometry* geom )
{
  if (mEditBuffer)
    return mEditBuffer->addTopologicalPoints( geom );
  else
    return -1;
}

int QgsVectorLayer::addTopologicalPoints( const QgsPoint& p )
{
  if (mEditBuffer)
    return mEditBuffer->addTopologicalPoints( p );
  else
    return -1;
}

int QgsVectorLayer::insertSegmentVerticesForSnap( const QList<QgsSnappingResult>& snapResults )
{
  if (mEditBuffer)
    return mEditBuffer->insertSegmentVerticesForSnap( snapResults );
  else
    return -1;
}

QgsVectorLayer::VertexMarkerType QgsVectorLayer::currentVertexMarkerType()
{
  QSettings settings;
  QString markerTypeString = settings.value( "/qgis/digitizing/marker_style", "Cross" ).toString();
  if ( markerTypeString == "Cross" )
  {
    return QgsVectorLayer::Cross;
  }
  else if ( markerTypeString == "SemiTransparentCircle" )
  {
    return QgsVectorLayer::SemiTransparentCircle;
  }
  else
  {
    return QgsVectorLayer::NoMarker;
  }
}

int QgsVectorLayer::currentVertexMarkerSize()
{
  QSettings settings;
  return settings.value( "/qgis/digitizing/marker_size", 3 ).toInt();
}

void QgsVectorLayer::drawFeature( QgsRenderContext &renderContext,
                                  QgsFeature& fet,
                                  QImage * marker )
{
  QPainter *p = renderContext.painter();
  // Only have variables, etc outside the switch() statement that are
  // used in all cases of the statement (otherwise they may get
  // executed, but never used, in a bit of code where performance is
  // critical).
  if ( ! fet.isValid() ) { return; }
  bool needToTrim = false;

  QgsGeometry* geom = fet.geometry();
  unsigned char* feature = geom->asWkb();

  QGis::WkbType wkbType = geom->wkbType();

  switch ( wkbType )
  {
    case QGis::WKBPoint:
    case QGis::WKBPoint25D:
    {
      double x = *(( double * )( feature + 5 ) );
      double y = *(( double * )( feature + 5 + sizeof( double ) ) );

      transformPoint( x, y, &renderContext.mapToPixel(), renderContext.coordinateTransform() );
      if ( std::abs( x ) > QgsClipper::MAX_X ||
           std::abs( y ) > QgsClipper::MAX_Y )
      {
        break;
      }

      //QPointF pt(x - (marker->width()/2),  y - (marker->height()/2));
      QPointF pt( x*renderContext.rasterScaleFactor() - ( marker->width() / 2 ),
                  y*renderContext.rasterScaleFactor() - ( marker->height() / 2 ) );

      p->save();
      //p->scale(markerScaleFactor,markerScaleFactor);
      p->scale( 1.0 / renderContext.rasterScaleFactor(), 1.0 / renderContext.rasterScaleFactor() );
      p->drawImage( pt, *marker );
      p->restore();

      break;
    }
    case QGis::WKBMultiPoint:
    case QGis::WKBMultiPoint25D:
    {
      unsigned char *ptr = feature + 5;
      unsigned int nPoints = *(( int* )ptr );
      ptr += 4;

      p->save();
      //p->scale(markerScaleFactor, markerScaleFactor);
      p->scale( 1.0 / renderContext.rasterScaleFactor(), 1.0 / renderContext.rasterScaleFactor() );

      for ( register unsigned int i = 0; i < nPoints; ++i )
      {
        ptr += 5;
        double x = *(( double * ) ptr );
        ptr += sizeof( double );
        double y = *(( double * ) ptr );
        ptr += sizeof( double );

        if ( wkbType == QGis::WKBMultiPoint25D ) // ignore Z value
          ptr += sizeof( double );

        transformPoint( x, y, &renderContext.mapToPixel(), renderContext.coordinateTransform() );
        //QPointF pt(x - (marker->width()/2),  y - (marker->height()/2));
        //QPointF pt(x/markerScaleFactor - (marker->width()/2),  y/markerScaleFactor - (marker->height()/2));
        QPointF pt( x*renderContext.rasterScaleFactor() - ( marker->width() / 2 ),
                    y*renderContext.rasterScaleFactor() - ( marker->height() / 2 ) );
        //QPointF pt( x, y );

        // Work around a +/- 32768 limitation on coordinates
        if ( std::abs( x ) > QgsClipper::MAX_X ||
             std::abs( y ) > QgsClipper::MAX_Y )
          needToTrim = true;
        else
          p->drawImage( pt, *marker );
      }
      p->restore();

      break;
    }
    case QGis::WKBLineString:
    case QGis::WKBLineString25D:
    {
      drawLineString( feature, renderContext );
      break;
    }
    case QGis::WKBMultiLineString:
    case QGis::WKBMultiLineString25D:
    {
      unsigned char* ptr = feature + 5;
      unsigned int numLineStrings = *(( int* )ptr );
      ptr = feature + 9;

      for ( register unsigned int jdx = 0; jdx < numLineStrings; jdx++ )
      {
        ptr = drawLineString( ptr, renderContext );
      }
      break;
    }
    case QGis::WKBPolygon:
    case QGis::WKBPolygon25D:
    {
      drawPolygon( feature, renderContext );
      break;
    }
    case QGis::WKBMultiPolygon:
    case QGis::WKBMultiPolygon25D:
    {
      unsigned char *ptr = feature + 5;
      unsigned int numPolygons = *(( int* )ptr );
      ptr = feature + 9;
      for ( register unsigned int kdx = 0; kdx < numPolygons; kdx++ )
        ptr = drawPolygon( ptr, renderContext );
      break;
    }
    default:
      QgsDebugMsg( "Unknown WkbType ENCOUNTERED" );
      break;
  }
}



void QgsVectorLayer::setCoordinateSystem()
{
  QgsDebugMsg( "----- Computing Coordinate System" );

  //
  // Get the layers project info and set up the QgsCoordinateTransform
  // for this layer
  //

  // get CRS directly from provider
  *mCRS = mDataProvider->crs();

  //QgsCoordinateReferenceSystem provides a mechanism for FORCE a srs to be valid
  //which is inolves falling back to system, project or user selected
  //defaults if the srs is not properly intialised.
  //we only nee to do that if the srs is not alreay valid
  if ( !mCRS->isValid() )
  {
    mCRS->validate();
  }
}

// Convenience function to transform the given point
inline void QgsVectorLayer::transformPoint(
  double& x, double& y,
  const QgsMapToPixel* mtp,
  const QgsCoordinateTransform* ct )
{
  // transform the point
  if ( ct )
  {
    double z = 0;
    ct->transformInPlace( x, y, z );
  }

  // transform from projected coordinate system to pixel
  // position on map canvas
  mtp->transformInPlace( x, y );
}

inline void QgsVectorLayer::transformPoints(
  std::vector<double>& x, std::vector<double>& y, std::vector<double>& z,
  QgsRenderContext &renderContext )
{
  // transform the point
  if ( renderContext.coordinateTransform() )
    renderContext.coordinateTransform()->transformInPlace( x, y, z );

  // transform from projected coordinate system to pixel
  // position on map canvas
  renderContext.mapToPixel().transformInPlace( x, y );
}


const QString QgsVectorLayer::displayField() const
{
  return mDisplayField;
}

bool QgsVectorLayer::isEditable() const
{
  return ( mEditBuffer && mDataProvider );
}

bool QgsVectorLayer::isModified() const
{
  return (mEditBuffer && mEditBuffer->isModified());
}

void QgsVectorLayer::setModified( bool modified, bool onlyGeometry )
{
  if (mEditBuffer)
    mEditBuffer->setModified(modified, onlyGeometry);
}

QgsVectorLayer::EditType QgsVectorLayer::editType( int idx )
{
  const QgsFieldMap &fields = pendingFields();
  if ( fields.contains( idx ) && mEditTypes.contains( fields[idx].name() ) )
    return mEditTypes[ fields[idx].name()];
  else
    return LineEdit;
}

void QgsVectorLayer::setEditType( int idx, EditType type )
{
  const QgsFieldMap &fields = pendingFields();
  if ( fields.contains( idx ) )
    mEditTypes[ fields[idx].name()] = type;
}

QString QgsVectorLayer::editForm()
{
  return mEditForm;
}

void QgsVectorLayer::setEditForm( QString ui )
{
  mEditForm = ui;
}

void QgsVectorLayer::setAnnotationForm( const QString& ui )
{
  mAnnotationForm = ui;
}

QString QgsVectorLayer::editFormInit()
{
  return mEditFormInit;
}

void QgsVectorLayer::setEditFormInit( QString function )
{
  mEditFormInit = function;
}

QMap< QString, QVariant > &QgsVectorLayer::valueMap( int idx )
{
  const QgsFieldMap &fields = pendingFields();

  // FIXME: throw an exception!?
  if ( !fields.contains( idx ) )
  {
    QgsDebugMsg( QString( "field %1 not found" ).arg( idx ) );
  }

  if ( !mValueMaps.contains( fields[idx].name() ) )
    mValueMaps[ fields[idx].name()] = QMap<QString, QVariant>();

  return mValueMaps[ fields[idx].name()];
}

QgsVectorLayer::RangeData &QgsVectorLayer::range( int idx )
{
  const QgsFieldMap &fields = pendingFields();

  // FIXME: throw an exception!?
  if ( fields.contains( idx ) )
  {
    QgsDebugMsg( QString( "field %1 not found" ).arg( idx ) );
  }

  if ( !mRanges.contains( fields[idx].name() ) )
    mRanges[ fields[idx].name()] = RangeData();

  return mRanges[ fields[idx].name()];
}

void QgsVectorLayer::addOverlay( QgsVectorOverlay* overlay )
{
  mOverlays.push_back( overlay );
}

void QgsVectorLayer::removeOverlay( const QString& typeName )
{
  for ( int i = mOverlays.size() - 1; i >= 0; --i )
  {
    if ( mOverlays.at( i )->typeName() == typeName )
    {
      mOverlays.removeAt( i );
    }
  }
}

void QgsVectorLayer::vectorOverlays( QList<QgsVectorOverlay*>& overlayList )
{
  overlayList = mOverlays;
}

QgsVectorOverlay* QgsVectorLayer::findOverlayByType( const QString& typeName )
{
  QList<QgsVectorOverlay*>::iterator it = mOverlays.begin();
  for ( ; it != mOverlays.end(); ++it )
  {
    if (( *it )->typeName() == typeName )
    {
      return *it;
    }
  }
  return 0; //not found
}


QgsFeatureRendererV2* QgsVectorLayer::rendererV2()
{
  return mRendererV2;
}
void QgsVectorLayer::setRendererV2( QgsFeatureRendererV2* r )
{
  delete mRendererV2;
  mRendererV2 = r;
}
bool QgsVectorLayer::isUsingRendererV2()
{
  return mUsingRendererV2;
}
void QgsVectorLayer::setUsingRendererV2( bool usingRendererV2 )
{
  mUsingRendererV2 = usingRendererV2;
}



void QgsVectorLayer::beginEditCommand( QString text )
{
  if (mEditBuffer)
    mEditBuffer->beginEditCommand(text);
}

void QgsVectorLayer::endEditCommand()
{
  if (mEditBuffer)
    mEditBuffer->endEditCommand();
}

void QgsVectorLayer::destroyEditCommand()
{
  if (mEditBuffer)
    mEditBuffer->destroyEditCommand();
}

void QgsVectorLayer::redoEditCommand( QgsUndoCommand* cmd )
{
  if (mEditBuffer)
    mEditBuffer->redoEditCommand(cmd);
}

void QgsVectorLayer::undoEditCommand( QgsUndoCommand* cmd )
{
  if (mEditBuffer)
    mEditBuffer->undoEditCommand(cmd);
}

void QgsVectorLayer::setCheckedState( int idx, QString checked, QString unchecked )
{
  const QgsFieldMap &fields = pendingFields();
  if ( fields.contains( idx ) )
    mCheckedStates[ fields[idx].name()] = QPair<QString, QString>( checked, unchecked );
}

QPair<QString, QString> QgsVectorLayer::checkedState( int idx )
{
  const QgsFieldMap &fields = pendingFields();
  if ( fields.contains( idx ) && mCheckedStates.contains( fields[idx].name() ) )
    return mCheckedStates[ fields[idx].name()];
  else
    return QPair<QString, QString>( "1", "0" );
}

int QgsVectorLayer::fieldNameIndex( const QString& fieldName ) const
{
  const QgsFieldMap &theFields = pendingFields();

  for ( QgsFieldMap::const_iterator it = theFields.constBegin(); it != theFields.constEnd(); ++it )
  {
    if ( it->name() == fieldName )
    {
      return it.key();
    }
  }
  return -1;
}

void QgsVectorLayer::stopRendererV2( QgsRenderContext& rendererContext, QgsSingleSymbolRendererV2* selRenderer )
{
  mRendererV2->stopRender( rendererContext );
  if ( selRenderer )
  {
    selRenderer->stopRender( rendererContext );
    delete selRenderer;
  }
}
