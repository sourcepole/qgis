#include "qgsgrassfeatureiterator.h"

#include "qgsgrassprovider.h"
#include "qgsapplication.h"
#include "qgslogger.h"

extern "C"
{
#include <grass/Vect.h>
}

// from provider:
// - isEdited(), isFrozen()
// - mLayers, mLayerId, mLayerType
// - mapOutdated(), updateMap(), update()
// - mMapVersion, mMaps, mMap
// - attributesOutdated(), loadAttributes()

QgsGrassFeatureIterator::QgsGrassFeatureIterator( QgsGrassProvider* p,
                                                  QgsAttributeList fetchAttributes,
                                                  QgsRectangle rect,
                                                  bool fetchGeometry,
                                                  bool useIntersect )
: QgsVectorDataProviderIterator( fetchAttributes, rect, fetchGeometry, useIntersect ),
  P(p)
{

  if ( P->isEdited() || P->isFrozen() || !P->mValid )
  {
    mClosed = true;
    return;
  }

  P->mMapMutex.lock();

  // Init structures
  mPoints = Vect_new_line_struct();
  mCats = Vect_new_cats_struct();
  mList = Vect_new_list();

  // check if outdated and update if necessary
  int mapId = P->mLayers[P->mLayerId].mapId;
  if ( P->mapOutdated( mapId ) )
  {
    P->updateMap( mapId );
  }
  if ( P->mMapVersion < P->mMaps[mapId].version )
  {
    P->update();
  }
  if ( P->attributesOutdated( mapId ) )
  {
    P->loadAttributes( P->mLayers[P->mLayerId] );
  }

  mNextCidx = 0;

  // Create selection array
  allocateSelection();

  //no selection rectangle - use all features
  if ( rect.isEmpty() )
  {
    resetSelection( 1 );
    return;
  }

  //apply selection rectangle
  resetSelection( 0 );

  if ( !useIntersect )
  { // select by bounding boxes only
    BOUND_BOX box;
    box.N = rect.yMaximum(); box.S = rect.yMinimum();
    box.E = rect.xMaximum(); box.W = rect.xMinimum();
    box.T = PORT_DOUBLE_MAX; box.B = -PORT_DOUBLE_MAX;
    if ( P->mLayerType == QgsGrassProvider::POINT ||
         P->mLayerType == QgsGrassProvider::CENTROID ||
         P->mLayerType == QgsGrassProvider::LINE ||
         P->mLayerType == QgsGrassProvider::BOUNDARY )
    {
      Vect_select_lines_by_box( P->mMap, &box, P->mGrassType, mList );
    }
    else if ( P->mLayerType == QgsGrassProvider::POLYGON )
    {
      Vect_select_areas_by_box( P->mMap, &box, mList );
    }

  }
  else
  { // check intersection
    struct line_pnts *Polygon;

    Polygon = Vect_new_line_struct();

    // Using z coor -PORT_DOUBLE_MAX/PORT_DOUBLE_MAX we cover 3D, Vect_select_lines_by_polygon is
    // using dig_line_box to get the box, it is not perfect, Vect_select_lines_by_polygon
    // should clarify better how 2D/3D is treated
    Vect_append_point( Polygon, rect.xMinimum(), rect.yMinimum(), -PORT_DOUBLE_MAX );
    Vect_append_point( Polygon, rect.xMaximum(), rect.yMinimum(), PORT_DOUBLE_MAX );
    Vect_append_point( Polygon, rect.xMaximum(), rect.yMaximum(), 0 );
    Vect_append_point( Polygon, rect.xMinimum(), rect.yMaximum(), 0 );
    Vect_append_point( Polygon, rect.xMinimum(), rect.yMinimum(), 0 );

    if ( P->mLayerType == QgsGrassProvider::POINT ||
         P->mLayerType == QgsGrassProvider::CENTROID ||
         P->mLayerType == QgsGrassProvider::LINE ||
         P->mLayerType == QgsGrassProvider::BOUNDARY )
    {
      Vect_select_lines_by_polygon( P->mMap, Polygon, 0, NULL, P->mGrassType, mList );
    }
    else if ( P->mLayerType == QgsGrassProvider::POLYGON )
    {
      Vect_select_areas_by_polygon( P->mMap, Polygon, 0, NULL, mList );
    }

    Vect_destroy_line_struct( Polygon );
  }

  for ( int i = 0; i < mList->n_values; i++ )
  {
    if ( mList->value[i] <= mSelectionSize )
    {
      mSelection[mList->value[i]] = 1;
    }
    else
    {
      QgsDebugMsg( "Selected element out of range" );
    }
  }

}

void QgsGrassFeatureIterator::allocateSelection()
{
  //int size;
  QgsDebugMsg( "entered." );

  int nlines = Vect_get_num_lines( P->mMap );
  int nareas = Vect_get_num_areas( P->mMap );

  if ( nlines > nareas )
  {
    mSelectionSize = nlines + 1;
  }
  else
  {
    mSelectionSize = nareas + 1;
  }
  QgsDebugMsg( QString( "nlines = %1 nareas = %2 size = %3" ).arg( nlines ).arg( nareas ).arg( mSelectionSize ) );

  mSelection = ( char * ) malloc( mSelectionSize );
}


void QgsGrassFeatureIterator::resetSelection( bool sel )
{
  QgsDebugMsg( "entered." );
  if ( !P->mValid ) return;
  memset( mSelection, ( int ) sel, mSelectionSize );
  mNextCidx = 0;
}



QgsGrassFeatureIterator::~QgsGrassFeatureIterator()
{
  close();
}

bool QgsGrassFeatureIterator::nextFeature(QgsFeature& feature)
{
  if (mClosed)
    return false;

  feature.setValid( false );
  int cat = -1, type = -1, id = -1;
  unsigned char *wkb;
  int wkbsize;

  QgsDebugMsgLevel( "entered.", 3 );

  if ( P->isEdited() || P->isFrozen() || !P->mValid )
    return false;

  if ( P->mCidxFieldIndex < 0 || mNextCidx >= P->mCidxFieldNumCats )
    return false; // No features, no features in this layer

  // Get next line/area id
  int found = 0;
  while ( mNextCidx < P->mCidxFieldNumCats )
  {
    Vect_cidx_get_cat_by_index( P->mMap, P->mCidxFieldIndex, mNextCidx++, &cat, &type, &id );
    // Warning: selection array is only of type line/area of current layer -> check type first

    if ( !( type & P->mGrassType ) )
      continue;

    if ( !mSelection[id] )
      continue;

    found = 1;
    break;
  }

  if ( !found )
    return false; // No more features

#if QGISDEBUG > 3
  QgsDebugMsg( QString( "cat = %1 type = %2 id = %3" ).arg( cat ).arg( type ).arg( id ) );
#endif

  feature.setFeatureId( id );
  feature.clearAttributeMap();

  // TODO int may be 64 bits (memcpy)
  if ( type & ( GV_POINTS | GV_LINES ) ) /* points or lines */
  {
    Vect_read_line( P->mMap, mPoints, mCats, id );
    int npoints = mPoints->n_points;

    if ( type & GV_POINTS )
    {
      wkbsize = 1 + 4 + 2 * 8;
    }
    else   // GV_LINES
    {
      wkbsize = 1 + 4 + 4 + npoints * 2 * 8;
    }
    wkb = new unsigned char[wkbsize];
    unsigned char *wkbp = wkb;
    wkbp[0] = ( unsigned char ) QgsApplication::endian();
    wkbp += 1;

    /* WKB type */
    memcpy( wkbp, &P->mQgisType, 4 );
    wkbp += 4;

    /* number of points */
    if ( type & GV_LINES )
    {
      memcpy( wkbp, &npoints, 4 );
      wkbp += 4;
    }

    for ( int i = 0; i < npoints; i++ )
    {
      memcpy( wkbp, &( mPoints->x[i] ), 8 );
      memcpy( wkbp + 8, &( mPoints->y[i] ), 8 );
      wkbp += 16;
    }
  }
  else   // GV_AREA
  {
    Vect_get_area_points( P->mMap, id, mPoints );
    int npoints = mPoints->n_points;

    wkbsize = 1 + 4 + 4 + 4 + npoints * 2 * 8; // size without islands
    wkb = new unsigned char[wkbsize];
    wkb[0] = ( unsigned char ) QgsApplication::endian();
    int offset = 1;

    /* WKB type */
    memcpy( wkb + offset, &P->mQgisType, 4 );
    offset += 4;

    /* Number of rings */
    int nisles = Vect_get_area_num_isles( P->mMap, id );
    int nrings = 1 + nisles;
    memcpy( wkb + offset, &nrings, 4 );
    offset += 4;

    /* Outer ring */
    memcpy( wkb + offset, &npoints, 4 );
    offset += 4;
    for ( int i = 0; i < npoints; i++ )
    {
      memcpy( wkb + offset, &( mPoints->x[i] ), 8 );
      memcpy( wkb + offset + 8, &( mPoints->y[i] ), 8 );
      offset += 16;
    }

    /* Isles */
    for ( int i = 0; i < nisles; i++ )
    {
      Vect_get_isle_points( P->mMap, Vect_get_area_isle( P->mMap, id, i ), mPoints );
      npoints = mPoints->n_points;

      // add space
      wkbsize += 4 + npoints * 2 * 8;
      wkb = ( unsigned char * ) realloc( wkb, wkbsize );

      memcpy( wkb + offset, &npoints, 4 );
      offset += 4;
      for ( int i = 0; i < npoints; i++ )
      {
        memcpy( wkb + offset, &( mPoints->x[i] ), 8 );
        memcpy( wkb + offset + 8, &( mPoints->y[i] ), 8 );
        offset += 16;
      }
    }
  }

  feature.setGeometryAndOwnership( wkb, wkbsize );

  P->setFeatureAttributes( P->mLayerId, cat, &feature, mFetchAttributes );

  feature.setValid( true );

  return true;
}

bool QgsGrassFeatureIterator::rewind()
{
  if (mClosed)
    return false;

  if ( P->isEdited() || P->isFrozen() || !P->mValid )
    return false;

  int mapId = P->mLayers[P->mLayerId].mapId;
  if ( P->mapOutdated( mapId ) )
  {
    P->updateMap( mapId );
  }
  if ( P->mMapVersion < P->mMaps[mapId].version )
  {
    P->update();

    // Create selection array
    free(mSelection);
    allocateSelection();

  }
  if ( P->attributesOutdated( mapId ) )
  {
    P->loadAttributes( P->mLayers[P->mLayerId] );
  }

  mNextCidx = 0;
  return true;
}

bool QgsGrassFeatureIterator::close()
{
  if (mClosed)
    return false;

  // finalization
  Vect_destroy_line_struct( mPoints );
  Vect_destroy_cats_struct( mCats );
  Vect_destroy_list( mList );

  P->mMapMutex.unlock();

  mClosed = true;
  return true;
}
