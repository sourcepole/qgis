#include "qgsosmfeatureiterator.h"
#include "osmprovider.h"

#include "qgsgeometry.h"
#include "qgslogger.h"

// from provider:
// - mFeatureType
// - mSelectFeatsStmt, mSelectFeatsInStmt
// - fetchNode(), fetchWay()

QgsOSMFeatureIterator::QgsOSMFeatureIterator( QgsOSMDataProvider* p,
                       QgsAttributeList fetchAttributes,
                       QgsRectangle rect,
                       bool fetchGeometry,
                       bool useIntersect )
 : QgsVectorDataProviderIterator(fetchAttributes, rect, fetchGeometry, useIntersect),
   P(p),
   mSelectionRectangleGeom( NULL )
{

  P->mDatabaseMutex.lock();

  mSelectionRectangleGeom = QgsGeometry::fromRect( rect );

  if ( rect.isEmpty() )
  {
    // we want to select all features from OSM data; we will use mSelectFeatsStmt
    // sqlite3 statement that is well prepared for this purpose
    mDatabaseStmt = P->mSelectFeatsStmt;
    return;
  }

  // we want to select features from specified boundary; we will use mSelectFeatsInStmt
  // sqlite3 statement that is well prepared for this purpose
  mDatabaseStmt = P->mSelectFeatsInStmt;

  if ( P->mFeatureType == QgsOSMDataProvider::PointType )
  {
    // binding variables (boundary) for points selection!
    sqlite3_bind_double( mDatabaseStmt, 1, rect.yMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 2, rect.yMaximum() );
    sqlite3_bind_double( mDatabaseStmt, 3, rect.xMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 4, rect.xMaximum() );
  }
  else if ( P->mFeatureType == QgsOSMDataProvider::LineType )
  {
    // binding variables (boundary) for lines selection!
    sqlite3_bind_double( mDatabaseStmt, 1, rect.yMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 2, rect.yMaximum() );
    sqlite3_bind_double( mDatabaseStmt, 3, rect.yMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 4, rect.yMaximum() );
    sqlite3_bind_double( mDatabaseStmt, 5, rect.yMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 6, rect.yMaximum() );

    sqlite3_bind_double( mDatabaseStmt, 7, rect.xMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 8, rect.xMaximum() );
    sqlite3_bind_double( mDatabaseStmt, 9, rect.xMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 10, rect.xMaximum() );
    sqlite3_bind_double( mDatabaseStmt, 11, rect.xMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 12, rect.xMaximum() );
  }
  else // P->mFeatureType == QgsOSMDataProvider::PolygonType
  {
    // binding variables (boundary) for polygons selection!
    sqlite3_bind_double( mDatabaseStmt, 1, rect.yMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 2, rect.yMaximum() );
    sqlite3_bind_double( mDatabaseStmt, 3, rect.yMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 4, rect.yMaximum() );
    sqlite3_bind_double( mDatabaseStmt, 5, rect.yMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 6, rect.yMaximum() );

    sqlite3_bind_double( mDatabaseStmt, 7, rect.xMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 8, rect.xMaximum() );
    sqlite3_bind_double( mDatabaseStmt, 9, rect.xMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 10, rect.xMaximum() );
    sqlite3_bind_double( mDatabaseStmt, 11, rect.xMinimum() );
    sqlite3_bind_double( mDatabaseStmt, 12, rect.xMaximum() );
  }

}

QgsOSMFeatureIterator::~QgsOSMFeatureIterator()
{
  close();
}

bool QgsOSMFeatureIterator::nextFeature(QgsFeature& feature)
{
  if (mClosed)
    return false;

  // load next requested feature from sqlite3 database
  switch ( sqlite3_step( mDatabaseStmt ) )
  {
    case SQLITE_DONE:  // no more features to return
      feature.setValid( false );
      return false;

    case SQLITE_ROW:  // another feature to return
      if ( P->mFeatureType == QgsOSMDataProvider::PointType )
        return P->fetchNode( feature, mDatabaseStmt, mFetchGeometry, mFetchAttributes );
      else if ( P->mFeatureType == QgsOSMDataProvider::LineType )
        return P->fetchWay( feature, mDatabaseStmt, mFetchGeometry, mFetchAttributes, mSelectionRectangleGeom );
      else if ( P->mFeatureType == QgsOSMDataProvider::PolygonType )
        return P->fetchWay( feature, mDatabaseStmt, mFetchGeometry, mFetchAttributes, mSelectionRectangleGeom );

    default:
      if ( P->mFeatureType == QgsOSMDataProvider::PointType )
        QgsDebugMsg( "Getting next feature of type <point> failed." );
      else if ( P->mFeatureType == QgsOSMDataProvider::LineType )
        QgsDebugMsg( "Getting next feature of type <line> failed." );
      else if ( P->mFeatureType == QgsOSMDataProvider::PolygonType )
        QgsDebugMsg( "Getting next feature of type <polygon> failed." );
      feature.setValid( false );
      return false;
  }

}

bool QgsOSMFeatureIterator::rewind()
{
  if (mClosed)
    return false;

  // we have to reset precompiled database statement; thanx to this action the first feature
  // (returned by the query) will be selected again with the next calling of sqlite3_step(mDatabaseStmt)
  if ( mDatabaseStmt )
    sqlite3_reset( mDatabaseStmt );

  return true;
}

bool QgsOSMFeatureIterator::close()
{
  if (mClosed)
    return false;

  // we must reset sqlite3 statement after recent selection - make it ready for next features selection
  if ( mDatabaseStmt )
    sqlite3_reset( mDatabaseStmt );

  // destruct selected geometry
  delete mSelectionRectangleGeom;
  mSelectionRectangleGeom = NULL;

  P->mDatabaseMutex.unlock();

  mClosed = true;
  return true;
}
