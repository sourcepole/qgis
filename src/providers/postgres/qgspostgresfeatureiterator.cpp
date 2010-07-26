#include "qgspostgresfeatureiterator.h"

#include "qgspostgresprovider.h"

#include "qgslogger.h"

// used from provider:
// - providerId
// - connectionRO
// - geometryColumn
// - srid
// - sqlWhereClause
// - featuresCounted
// - declareCursor()
// - getFeature()
// - quotedIdentifier()

QgsPostgresFeatureIterator::QgsPostgresFeatureIterator( QgsPostgresProvider* p,
                            QgsAttributeList fetchAttributes,
                            QgsRectangle rect,
                            bool fetchGeometry,
                            bool useIntersect )
: QgsVectorDataProviderIterator(fetchAttributes, rect, fetchGeometry, useIntersect),
  P(p)
  , mFeatureQueueSize( 200 )
{
  P->mConnectionROMutex.lock();

  mCursorName = QString( "qgisf%1" ).arg( P->providerId );

  QString whereClause;

  if ( !rect.isEmpty() )
  {
    if ( useIntersect )
    {
      // Contributed by #qgis irc "creeping"
      // This version actually invokes PostGIS's use of spatial indexes
      whereClause = QString( "%1 && setsrid('BOX3D(%2)'::box3d,%3) and intersects(%1,setsrid('BOX3D(%2)'::box3d,%3))" )
                    .arg( P->quotedIdentifier( P->geometryColumn ) )
                    .arg( rect.asWktCoordinates() )
                    .arg( P->srid );
    }
    else
    {
      whereClause = QString( "%1 && setsrid('BOX3D(%2)'::box3d,%3)" )
                    .arg( P->quotedIdentifier( P->geometryColumn ) )
                    .arg( rect.asWktCoordinates() )
                    .arg( P->srid );
    }
  }

  if ( !P->sqlWhereClause.isEmpty() )
  {
    if ( !whereClause.isEmpty() )
      whereClause += " and ";

    whereClause += "(" + P->sqlWhereClause + ")";
  }

  if ( !P->declareCursor( mCursorName, fetchAttributes, fetchGeometry, whereClause ) )
  {
    mClosed = true;
    return;
  }

  mFetched = 0;
}

QgsPostgresFeatureIterator::~QgsPostgresFeatureIterator()
{
  close();
}


bool QgsPostgresFeatureIterator::nextFeature(QgsFeature& feature)
{
  feature.setValid( false );

  if ( mClosed )
    return false;

  if ( mFeatureQueue.empty() )
  {
    QString fetch = QString( "fetch forward %1 from %2" ).arg( mFeatureQueueSize ).arg( mCursorName );
    if ( P->connectionRO->PQsendQuery( fetch ) == 0 ) // fetch features asynchronously
    {
      QgsDebugMsg( "PQsendQuery failed" );
    }

    QgsPostgresProvider::Result queryResult;
    while (( queryResult = P->connectionRO->PQgetResult() ) )
    {
      int rows = PQntuples( queryResult );
      if ( rows == 0 )
        continue;

      for ( int row = 0; row < rows; row++ )
      {
        mFeatureQueue.push( QgsFeature() );
        P->getFeature( queryResult, row, mFetchGeometry, mFeatureQueue.back(), mFetchAttributes );
      } // for each row in queue
    }
  }

  if ( mFeatureQueue.empty() )
  {
    QgsDebugMsg( QString( "finished after %1 features" ).arg( mFetched ) );

    close();

    if ( P->featuresCounted < mFetched )
    {
      QgsDebugMsg( QString( "feature count adjusted from %1 to %2" ).arg( P->featuresCounted ).arg( mFetched ) );
      P->featuresCounted = mFetched;
    }
    return false;
  }

  // Now return the next feature from the queue
  if ( mFetchGeometry )
  {
    QgsGeometry* featureGeom = mFeatureQueue.front().geometryAndOwnership();
    feature.setGeometry( featureGeom );
  }
  else
  {
    feature.setGeometryAndOwnership( 0, 0 );
  }
  feature.setFeatureId( mFeatureQueue.front().id() );
  feature.setAttributeMap( mFeatureQueue.front().attributeMap() );

  mFeatureQueue.pop();
  mFetched++;

  feature.setValid( true );
  return true;
}

bool QgsPostgresFeatureIterator::rewind()
{
  if ( mClosed )
    return false;

  //move cursor to first record
  P->connectionRO->PQexecNR( QString( "move 0 in " ) + mCursorName );

  mFeatureQueue.empty();

  return true;
}

bool QgsPostgresFeatureIterator::close()
{
  if ( mClosed )
    return false;

  P->connectionRO->closeCursor( mCursorName );

  P->mConnectionROMutex.unlock();

  mClosed = true;
  return true;
}
