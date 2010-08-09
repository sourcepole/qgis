#include "qgsspatialitefeatureiterator.h"
#include "qgsspatialiteprovider.h"

#include <sqlite3.h>

#include "qgslogger.h"

// from provider:
// - sqliteHandle
// - mIndexTable, mIndexGeometry
// - mGeometryColumn, mTableName
// - mSubsetString, mVShapeBased
// - spatialIndexRTree, spatialIndexMbrCache
// - field()

QgsSpatiaLiteFeatureIterator::QgsSpatiaLiteFeatureIterator( QgsSpatiaLiteProvider* p,
                                                            QgsAttributeList fetchAttributes,
                                                            QgsRectangle rect,
                                                            bool fetchGeometry,
                                                            bool useIntersect )
 : QgsVectorDataProviderIterator(fetchAttributes, rect, fetchGeometry, useIntersect),
   P(p),
   sqliteStatement( NULL )
{
  P->mHandleMutex.lock();

  // prepare the SQL statement:
  // - first column is always ROWID
  // - next columns are attributes (if required)
  // - last column is geometry (if required)

  QString sql = "SELECT ROWID";
  for ( QgsAttributeList::const_iterator it = fetchAttributes.constBegin(); it != fetchAttributes.constEnd(); ++it )
  {
    const QgsField & fld = P->field( *it );
    const QString & fieldname = fld.name();
    sql += "," + QgsSpatiaLiteProvider::quotedIdentifier( fieldname );
  }
  if ( fetchGeometry )
  {
    sql += QString( ", AsBinary(%1)" ).arg( QgsSpatiaLiteProvider::quotedIdentifier( P->mGeometryColumn ) );
  }
  sql += QString( " FROM %1" ).arg( QgsSpatiaLiteProvider::quotedIdentifier( P->mTableName ) );

  QString whereClause;

  if ( !rect.isEmpty() )
  {
    // some kind of MBR spatial filtering is required
    whereClause = " WHERE ";
    if ( useIntersect )
    {
      // we are requested to evaluate a true INTERSECT relationship
      QString mbr = QString( "%1, %2, %3, %4" ).
                    arg( QString::number( rect.xMinimum(), 'f', 6 ) ).
                    arg( QString::number( rect.yMinimum(), 'f', 6 ) ).
                    arg( QString::number( rect.xMaximum(), 'f', 6 ) ).arg( QString::number( rect.yMaximum(), 'f', 6 ) );
      whereClause += QString( "Intersects(%1, BuildMbr(%2)) AND " ).arg( QgsSpatiaLiteProvider::quotedIdentifier( P->mGeometryColumn ) ).arg( mbr );
    }
    if ( P->mVShapeBased )
    {
      // handling a VirtualShape layer
      QString mbr = QString( "%1, %2, %3, %4" ).
                    arg( QString::number( rect.xMinimum(), 'f', 6 ) ).
                    arg( QString::number( rect.yMinimum(), 'f', 6 ) ).
                    arg( QString::number( rect.xMaximum(), 'f', 6 ) ).arg( QString::number( rect.yMaximum(), 'f', 6 ) );
      whereClause += QString( "MbrIntersects(%1, BuildMbr(%2))" ).arg( QgsSpatiaLiteProvider::quotedIdentifier( P->mGeometryColumn ) ).arg( mbr );
    }
    else
    {
      if ( P->spatialIndexRTree )
      {
        // using the RTree spatial index
        QString mbrFilter = QString( "xmin <= %1 AND " ).arg( QString::number( rect.xMaximum(), 'f', 6 ) );
        mbrFilter += QString( "xmax >= %1 AND " ).arg( QString::number( rect.xMinimum(), 'f', 6 ) );
        mbrFilter += QString( "ymin <= %1 AND " ).arg( QString::number( rect.yMaximum(), 'f', 6 ) );
        mbrFilter += QString( "ymax >= %1" ).arg( QString::number( rect.yMinimum(), 'f', 6 ) );
        QString idxName = QString( "idx_%1_%2" ).arg( P->mIndexTable ).arg( P->mIndexGeometry );
        whereClause += QString( "ROWID IN (SELECT pkid FROM %1 WHERE %2)" ).arg( QgsSpatiaLiteProvider::quotedIdentifier( idxName ) ).arg( mbrFilter );
      }
      else if ( P->spatialIndexMbrCache )
      {
        // using the MbrCache spatial index
        QString mbr = QString( "%1, %2, %3, %4" ).
                      arg( QString::number( rect.xMinimum(), 'f', 6 ) ).
                      arg( QString::number( rect.yMinimum(), 'f', 6 ) ).
                      arg( QString::number( rect.xMaximum(), 'f', 6 ) ).arg( QString::number( rect.yMaximum(), 'f', 6 ) );
        QString idxName = QString( "cache_%1_%2" ).arg( P->mIndexTable ).arg( P->mIndexGeometry );
        whereClause += QString( "ROWID IN (SELECT rowid FROM %1 WHERE mbr = FilterMbrIntersects(%2))" ).arg( QgsSpatiaLiteProvider::quotedIdentifier( idxName ) ).arg( mbr );
      }
      else
      {
        // using simple MBR filtering
        QString mbr = QString( "%1, %2, %3, %4" ).
                      arg( QString::number( rect.xMinimum(), 'f', 6 ) ).
                      arg( QString::number( rect.yMinimum(), 'f', 6 ) ).
                      arg( QString::number( rect.xMaximum(), 'f', 6 ) ).arg( QString::number( rect.yMaximum(), 'f', 6 ) );
        whereClause += QString( "MbrIntersects(%1, BuildMbr(%2))" ).arg( QgsSpatiaLiteProvider::quotedIdentifier( P->mGeometryColumn ) ).arg( mbr );
      }
    }
  }

  if ( !whereClause.isEmpty() )
    sql += whereClause;

  if ( !P->mSubsetString.isEmpty() )
  {
    if ( !whereClause.isEmpty() )
    {
      sql += " AND ";
    }
    else
    {
      sql += " WHERE ";
    }
    sql += "( " + P->mSubsetString + ")";
  }

  if ( sqlite3_prepare_v2( P->sqliteHandle, sql.toUtf8().constData(), -1, &sqliteStatement, NULL ) != SQLITE_OK )
  {
    // some error occurred
    QgsDebugMsg( QString( "SQLite error: %1\n\nSQL: %2" ).arg( sql ).arg( QString::fromUtf8( sqlite3_errmsg( P->sqliteHandle ) ) ) );
    sqliteStatement = NULL;
  }

}

QgsSpatiaLiteFeatureIterator::~QgsSpatiaLiteFeatureIterator()
{
  close();
}

bool QgsSpatiaLiteFeatureIterator::nextFeature(QgsFeature& feature)
{
  if (mClosed)
    return false;

  feature.setValid( false );

  if ( sqliteStatement == NULL )
  {
    QgsDebugMsg( "Invalid current SQLite statement" );
    return false;
  }

  int ret = sqlite3_step( sqliteStatement );
  if ( ret == SQLITE_DONE )
  {
    // there are no more rows to fetch - we can stop looping destroying the SQLite statement
    sqlite3_finalize( sqliteStatement );
    sqliteStatement = NULL;
    return false;
  }
  if ( ret == SQLITE_ROW )
  {
    // one valid row has been fetched from the result set
    P->getFeature( feature, sqliteStatement, mFetchAttributes, mFetchGeometry );
  }
  else
  {
    // some unexpected error occurred
    QgsDebugMsg( QString( "sqlite3_step() error: %1" ).arg( QString::fromUtf8( sqlite3_errmsg( P->sqliteHandle ) ) ) );
    sqlite3_finalize( sqliteStatement );
    sqliteStatement = NULL;
    return false;
  }

  feature.setValid( true );
  return true;

}

bool QgsSpatiaLiteFeatureIterator::rewind()
{
  if (mClosed)
    return false;

  if ( sqliteStatement )
  {
    sqlite3_finalize( sqliteStatement );
    sqliteStatement = NULL;
  }

  return true;
}

bool QgsSpatiaLiteFeatureIterator::close()
{
  if (mClosed)
    return false;

  P->mHandleMutex.unlock();

  mClosed = true;
  return true;
}
