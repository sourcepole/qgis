/***************************************************************************
    memoryprovider.cpp - provider with storage in memory
    ------------------
    begin                : June 2008
    copyright            : (C) 2008 by Martin Dobias
    email                : wonder.sk at gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmemoryprovider.h"
#include "qgsmemoryfeatureiterator.h"

#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsspatialindex.h"
#include "qgscoordinatereferencesystem.h"


static const QString TEXT_PROVIDER_KEY = "memory";
static const QString TEXT_PROVIDER_DESCRIPTION = "Memory provider";

QgsMemoryProvider::QgsMemoryProvider( QString uri )
    : QgsVectorDataProvider( uri ),
    mSpatialIndex( NULL )
{
  if ( uri == "Point" )
    mWkbType = QGis::WKBPoint;
  else if ( uri == "LineString" )
    mWkbType = QGis::WKBLineString;
  else if ( uri == "Polygon" )
    mWkbType = QGis::WKBPolygon;
  else if ( uri == "MultiPoint" )
    mWkbType = QGis::WKBMultiPoint;
  else if ( uri == "MultiLineString" )
    mWkbType = QGis::WKBMultiLineString;
  else if ( uri == "MultiPolygon" )
    mWkbType = QGis::WKBMultiPolygon;
  else
    mWkbType = QGis::WKBUnknown;

  mNextFeatureId = 1;

  mNativeTypes
  << QgsVectorDataProvider::NativeType( tr( "Whole number (integer)" ), "integer", QVariant::Int, 1, 10 )
  << QgsVectorDataProvider::NativeType( tr( "Decimal number (real)" ), "double", QVariant::Double, 1, 20, 0, 5 )
  << QgsVectorDataProvider::NativeType( tr( "Text (string)" ), "string", QVariant::String, 1, 255 )
  ;
}

QgsMemoryProvider::~QgsMemoryProvider()
{
  delete mSpatialIndex;
}

QString QgsMemoryProvider::storageType() const
{
  return "Memory storage";
}

QgsFeatureIterator QgsMemoryProvider::getFeatures( QgsAttributeList fetchAttributes,
                                                   QgsRectangle rect,
                                                   bool fetchGeometry,
                                                   bool useIntersect )
{
  return QgsFeatureIterator( new QgsMemoryFeatureIterator(this, fetchAttributes, rect, fetchGeometry, useIntersect) );
}

bool QgsMemoryProvider::nextFeature( QgsFeature& feature )
{
  if (mOldApiIter.nextFeature(feature))
    return true;
  else
  {
    mOldApiIter.close(); // make sure to unlock the layer
    return false;
  }
}


bool QgsMemoryProvider::featureAtId( int featureId,
                                     QgsFeature& feature,
                                     bool fetchGeometry,
                                     QgsAttributeList fetchAttributes )
{
  QMutexLocker locker(&mDataMutex);

  feature.setValid( false );
  QgsFeatureMap::iterator it = mFeatures.find( featureId );

  if ( it == mFeatures.end() )
    return false;

  feature = *it;
  feature.setValid( true );
  return true;
}


void QgsMemoryProvider::select( QgsAttributeList fetchAttributes,
                                QgsRectangle rect,
                                bool fetchGeometry,
                                bool useIntersect )
{
  mOldApiIter = getFeatures( fetchAttributes, rect, fetchGeometry, useIntersect );
}

void QgsMemoryProvider::rewind()
{
  mOldApiIter.rewind();
}


QgsRectangle QgsMemoryProvider::extent()
{
  return mExtent;
}

QGis::WkbType QgsMemoryProvider::geometryType() const
{
  return mWkbType;
}

long QgsMemoryProvider::featureCount() const
{
  return mFeatures.count();
}

uint QgsMemoryProvider::fieldCount() const
{
  return mFields.count();
}


const QgsFieldMap & QgsMemoryProvider::fields() const
{
  return mFields;
}

bool QgsMemoryProvider::isValid()
{
  return ( mWkbType != QGis::WKBUnknown );
}

QgsCoordinateReferenceSystem QgsMemoryProvider::crs()
{
  // TODO: make provider projection-aware
  return QgsCoordinateReferenceSystem(); // return default CRS
}


bool QgsMemoryProvider::addFeatures( QgsFeatureList & flist )
{
  QMutexLocker locker(&mDataMutex);

  // TODO: sanity checks of fields and geometries
  for ( QgsFeatureList::iterator it = flist.begin(); it != flist.end(); ++it )
  {
    mFeatures[mNextFeatureId] = *it;
    QgsFeature& newfeat = mFeatures[mNextFeatureId];
    newfeat.setFeatureId( mNextFeatureId );

    // update spatial index
    if ( mSpatialIndex )
      mSpatialIndex->insertFeature( newfeat );

    mNextFeatureId++;
  }


  updateExtent();

  return true;
}

bool QgsMemoryProvider::deleteFeatures( const QgsFeatureIds & id )
{
  QMutexLocker locker(&mDataMutex);

  for ( QgsFeatureIds::const_iterator it = id.begin(); it != id.end(); ++it )
  {
    QgsFeatureMap::iterator fit = mFeatures.find( *it );

    // check whether such feature exists
    if ( fit == mFeatures.end() )
      continue;

    // update spatial index
    if ( mSpatialIndex )
      mSpatialIndex->deleteFeature( *fit );

    mFeatures.erase( fit );
  }

  updateExtent();

  return true;
}

bool QgsMemoryProvider::addAttributes( const QList<QgsField> &attributes )
{
  QMutexLocker locker(&mDataMutex);

  for ( QList<QgsField>::const_iterator it = attributes.begin(); it != attributes.end(); ++it )
  {
    switch ( it->type() )
    {
      case QVariant::Int:
      case QVariant::Double:
      case QVariant::String:
        break;
      default:
        QgsDebugMsg( "Field type not supported: " + it->typeName() );
        continue;
    }

    // add new field as a last one
    int nextId = -1;
    for ( QgsFieldMap::iterator it2 = mFields.begin(); it2 != mFields.end(); ++it2 )
      if ( it2.key() > nextId ) nextId = it2.key();
    mFields[nextId+1] = *it;
  }
  return true;
}

bool QgsMemoryProvider::deleteAttributes( const QgsAttributeIds& attributes )
{
  QMutexLocker locker(&mDataMutex);

  for ( QgsAttributeIds::const_iterator it = attributes.begin(); it != attributes.end(); ++it )
    mFields.remove( *it );
  return true;
}

bool QgsMemoryProvider::changeAttributeValues( const QgsChangedAttributesMap & attr_map )
{
  QMutexLocker locker(&mDataMutex);

  for ( QgsChangedAttributesMap::const_iterator it = attr_map.begin(); it != attr_map.end(); ++it )
  {
    QgsFeatureMap::iterator fit = mFeatures.find( it.key() );
    if ( fit == mFeatures.end() )
      continue;

    const QgsAttributeMap& attrs = it.value();
    for ( QgsAttributeMap::const_iterator it2 = attrs.begin(); it2 != attrs.end(); ++it2 )
      fit->changeAttribute( it2.key(), it2.value() );
  }
  return true;
}

bool QgsMemoryProvider::changeGeometryValues( QgsGeometryMap & geometry_map )
{
  QMutexLocker locker(&mDataMutex);

  for ( QgsGeometryMap::const_iterator it = geometry_map.begin(); it != geometry_map.end(); ++it )
  {
    QgsFeatureMap::iterator fit = mFeatures.find( it.key() );
    if ( fit == mFeatures.end() )
      continue;

    // update spatial index
    if ( mSpatialIndex )
      mSpatialIndex->deleteFeature( *fit );

    fit->setGeometry( it.value() );

    // update spatial index
    if ( mSpatialIndex )
      mSpatialIndex->insertFeature( *fit );
  }

  updateExtent();

  return true;
}

bool QgsMemoryProvider::createSpatialIndex()
{
  QMutexLocker locker(&mDataMutex);

  if ( !mSpatialIndex )
  {
    mSpatialIndex = new QgsSpatialIndex();

    // add existing features to index
    for ( QgsFeatureMap::iterator it = mFeatures.begin(); it != mFeatures.end(); ++it )
    {
      mSpatialIndex->insertFeature( *it );
    }
  }
  return true;
}

int QgsMemoryProvider::capabilities() const
{
  return AddFeatures | DeleteFeatures | ChangeGeometries |
         ChangeAttributeValues | AddAttributes | DeleteAttributes | CreateSpatialIndex |
         SelectAtId | SelectGeometryAtId;
}


void QgsMemoryProvider::updateExtent()
{
  if ( mFeatures.count() == 0 )
  {
    mExtent = QgsRectangle();
  }
  else
  {
    mExtent = mFeatures.begin().value().geometry()->boundingBox();
    for ( QgsFeatureMap::iterator it = mFeatures.begin(); it != mFeatures.end(); ++it )
      mExtent.unionRect( it.value().geometry()->boundingBox() );
  }
}



// --------------------------------

QString  QgsMemoryProvider::name() const
{
  return TEXT_PROVIDER_KEY;
}

QString  QgsMemoryProvider::description() const
{
  return TEXT_PROVIDER_DESCRIPTION;
}

// --------------------------------


/**
 * Class factory to return a pointer to a newly created
 * QgsMemoryProvider object
 */
QGISEXTERN QgsMemoryProvider *classFactory( const QString *uri )
{
  return new QgsMemoryProvider( *uri );
}

/** Required key function (used to map the plugin to a data store type)
 */
QGISEXTERN QString providerKey()
{
  return TEXT_PROVIDER_KEY;
}

/**
 * Required description function
 */
QGISEXTERN QString description()
{
  return TEXT_PROVIDER_DESCRIPTION;
}

/**
 * Required isProvider function. Used to determine if this shared library
 * is a data provider plugin
 */
QGISEXTERN bool isProvider()
{
  return true;
}
