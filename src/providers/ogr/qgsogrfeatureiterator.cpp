#include "qgsogrfeatureiterator.h"

#include "qgsogrprovider.h"

#include "qgsapplication.h"
#include "qgslogger.h"
#include "qgsgeometry.h"

// P:
// - ogrLayer
// - mFetchFeaturesWithoutGeom
// - getFeatureAttribute()

QgsOgrFeatureIterator::QgsOgrFeatureIterator( QgsOgrProvider* p,
                                              QgsAttributeList fetchAttributes,
                                              QgsRectangle rect,
                                              bool fetchGeometry,
                                              bool useIntersect )
 : QgsVectorDataProviderIterator( fetchAttributes, rect, fetchGeometry, useIntersect ),
   P( p )
{
  // first of all, lock the OGR layer!
  QgsDebugMsg( "trying to lock OGR layer" );
  P->mLayerMutex.lock();

  // set the selection rectangle pointer to 0
  mSelectionRectangle = 0;


  // spatial query to select features
  if ( rect.isEmpty() )
  {
    OGR_L_SetSpatialFilter( P->ogrLayer, 0 );
  }
  else
  {
    OGRGeometryH filter = 0;
    QString wktExtent = QString( "POLYGON((%1))" ).arg( rect.asPolygon() );
    QByteArray ba = wktExtent.toAscii();
    const char *wktText = ba;

    if ( useIntersect )
    {
      // store the selection rectangle for use in filtering features during
      // an identify and display attributes
      if ( mSelectionRectangle )
        OGR_G_DestroyGeometry( mSelectionRectangle );

      OGR_G_CreateFromWkt(( char ** )&wktText, NULL, &mSelectionRectangle );
      wktText = ba;
    }

    OGR_G_CreateFromWkt(( char ** )&wktText, NULL, &filter );
    QgsDebugMsg( "Setting spatial filter using " + wktExtent );
    OGR_L_SetSpatialFilter( P->ogrLayer, filter );
    OGR_G_DestroyGeometry( filter );
  }

  P->setIgnoredFields( fetchGeometry, fetchAttributes );

  //start with first feature
  OGR_L_ResetReading( P->ogrLayer );

}

QgsOgrFeatureIterator::~QgsOgrFeatureIterator()
{
  close();
}


bool QgsOgrFeatureIterator::nextFeature( QgsFeature& feature )
{

  feature.setValid( false );

  OGRFeatureH fet;
  QgsRectangle selectionRect;

  while (( fet = OGR_L_GetNextFeature( P->ogrLayer ) ) != NULL )
  {
    // skip features without geometry
    if ( !P->mFetchFeaturesWithoutGeom && OGR_F_GetGeometryRef( fet ) == NULL )
    {
      OGR_F_Destroy( fet );
      continue;
    }

    feature.setFeatureId( OGR_F_GetFID( fet ) );

    /* fetch geometry */
    if ( mFetchGeometry || mUseIntersect )
    {
      OGRGeometryH geom = OGR_F_GetGeometryRef( fet );

      if ( geom == 0 )
      {
        OGR_F_Destroy( fet );
        continue;
      }

      // get the wkb representation
      unsigned char *wkb = new unsigned char[OGR_G_WkbSize( geom )];
      OGR_G_ExportToWkb( geom, ( OGRwkbByteOrder ) QgsApplication::endian(), wkb );

      QgsGeometry* g = feature.geometry();
      if ( !g )
        feature.setGeometryAndOwnership( wkb, OGR_G_WkbSize( geom ) );
      else
      {
        g->fromWkb( wkb, OGR_G_WkbSize( geom ) );
      }

      if ( mUseIntersect )
      {
        //precise test for intersection with search rectangle
        //first make QgsRectangle from OGRPolygon
        OGREnvelope env;
        memset( &env, 0, sizeof( env ) );
        if ( mSelectionRectangle )
          OGR_G_GetEnvelope( mSelectionRectangle, &env );
        if ( env.MinX != 0 || env.MinY != 0 || env.MaxX != 0 || env.MaxY != 0 ) //if envelope is invalid, skip the precise intersection test
        {
          selectionRect.set( env.MinX, env.MinY, env.MaxX, env.MaxY );
          if ( !feature.geometry()->intersects( selectionRect ) )
          {
            OGR_F_Destroy( fet );
            continue;
          }
        }

      }
    }

    QVariant* attrs = feature.resizeAttributeVector( P->fieldCount() );

    /* fetch attributes */
    for ( QgsAttributeList::iterator it = mFetchAttributes.begin(); it != mFetchAttributes.end(); ++it )
    {
      P->getFeatureAttribute( fet, feature, *it, attrs );
    }

    /* we have a feature, end this cycle */
    break;

  } /* while */

  if ( fet )
  {
    if ( OGR_F_GetGeometryRef( fet ) != NULL )
    {
      feature.setValid( true );
    }
    else
    {
      feature.setValid( false );
    }
    OGR_F_Destroy( fet );
    return true;
  }
  else
  {
    QgsDebugMsg( "Feature is null" );
    // probably should reset reading here
    OGR_L_ResetReading( P->ogrLayer );
    return false;
  }

}

bool QgsOgrFeatureIterator::rewind()
{
  OGR_L_ResetReading( P->ogrLayer );

  return true;
}

bool QgsOgrFeatureIterator::close()
{
  if ( mClosed )
    return false;

  if ( mSelectionRectangle )
  {
    OGR_G_DestroyGeometry( mSelectionRectangle );
    mSelectionRectangle = 0;
  }

  // we're done - unlock the mutex
  QgsDebugMsg( "unlocking OGR layer" );
  P->mLayerMutex.unlock();

  mClosed = true;
  return true;
}
