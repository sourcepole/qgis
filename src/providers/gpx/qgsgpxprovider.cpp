/***************************************************************************
      qgsgpxprovider.cpp  -  Data provider for GPS eXchange files
                             -------------------
    begin                : 2004-04-14
    copyright            : (C) 2004 by Lars Luthman
    email                : larsl@users.sourceforge.net

    Partly based on qgsdelimitedtextprovider.cpp, (C) 2004 Gary E. Sherman
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include <algorithm>
#include <iostream>
#include <limits>
#include <cstring>
#include <cmath>

// Changed #include <qapp.h> to <qapplication.h>. Apparently some
// debian distros do not include the qapp.h wrapper and the compilation
// fails. [gsherman]
#include <QApplication>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QObject>

#include "qgis.h"
#include "qgsapplication.h"
#include "qgsdataprovider.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgsgeometry.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsrectangle.h"
#include "qgsgpxprovider.h"
#include "qgsgpxfeatureiterator.h"
#include "gpsdata.h"
#include "qgslogger.h"


const QString GPX_KEY = "gpx";

const QString GPX_DESCRIPTION = QObject::tr( "GPS eXchange format provider" );


QgsGPXProvider::QgsGPXProvider( QString uri ) :
    QgsVectorDataProvider( uri )
{
  // assume that it won't work
  mValid = false;

  // we always use UTF-8
  mEncoding = QTextCodec::codecForName( "utf8" );

  // get the file name and the type parameter from the URI
  int fileNameEnd = uri.indexOf( '?' );
  if ( fileNameEnd == -1 || uri.mid( fileNameEnd + 1, 5 ) != "type=" )
  {
    QgsLogger::warning( tr( "Bad URI - you need to specify the feature type." ) );
    return;
  }
  QString typeStr = uri.mid( fileNameEnd + 6 );
  mFeatureType = ( typeStr == "waypoint" ? WaypointType :
                   ( typeStr == "route" ? RouteType : TrackType ) );

  // set up the attributes and the geometry type depending on the feature type
  mAttributeVector.append( QgsField( "name", QVariant::String, "text" ) );
  mAttributeVector.append( QgsField( "comment", QVariant::String, "text" ) );
  mAttributeVector.append( QgsField( "description", QVariant::String, "text" ) );
  mAttributeVector.append( QgsField( "source", QVariant::String, "text" ) );
  mAttributeVector.append( QgsField( "url", QVariant::String, "text" ) );
  mAttributeVector.append( QgsField( "url name", QVariant::String, "text" ) );

  if ( mFeatureType == WaypointType )
  {
    mAttributeVector.append( QgsField( "elevation", QVariant::Double, "double" ) );
    mAttributeVector.append( QgsField( "symbol", QVariant::String, "text" ) );
  }
  else if ( mFeatureType == RouteType || mFeatureType == TrackType )
  {
    mAttributeVector.append( QgsField( "number", QVariant::Int, "int" ) );
  }

  // construct QgsFieldMap from the field vector (legacy)
  for (int i = 0; i < mAttributeVector.count(); i++)
    mAttributeFields.insert(i, mAttributeVector[i]);

  mFileName = uri.left( fileNameEnd );

  // parse the file
  data = QgsGPSData::getData( mFileName );
  if ( data == 0 )
  {
    return;
  }

  mValid = true;
}


QgsGPXProvider::~QgsGPXProvider()
{
  mOldApiIter.close();

  QgsGPSData::releaseData( mFileName );
}


QString QgsGPXProvider::storageType() const
{
  return tr( "GPS eXchange file" );
}

int QgsGPXProvider::capabilities() const
{
  return QgsVectorDataProvider::AddFeatures |
         QgsVectorDataProvider::DeleteFeatures |
         QgsVectorDataProvider::ChangeAttributeValues;
}

QgsFeatureIterator QgsGPXProvider::getFeatures( QgsAttributeList fetchAttributes,
                                                QgsRectangle rect,
                                                bool fetchGeometry,
                                                bool useIntersect )
{
  return QgsFeatureIterator( new QgsGPXFeatureIterator(this, fetchAttributes, rect, fetchGeometry, useIntersect) );
}


// Return the extent of the layer
QgsRectangle QgsGPXProvider::extent()
{
  return data->getExtent();
}


/**
 * Return the feature type
 */
QGis::WkbType QgsGPXProvider::geometryType() const
{
  if ( mFeatureType == WaypointType )
    return QGis::WKBPoint;

  if ( mFeatureType == RouteType || mFeatureType == TrackType )
    return QGis::WKBLineString;

  return QGis::WKBUnknown;
}


/**
 * Return the feature type
 */
long QgsGPXProvider::featureCount() const
{
  if ( mFeatureType == WaypointType )
    return data->getNumberOfWaypoints();
  if ( mFeatureType == RouteType )
    return data->getNumberOfRoutes();
  if ( mFeatureType == TrackType )
    return data->getNumberOfTracks();
  return 0;
}


/**
 * Return the number of fields
 */
uint QgsGPXProvider::fieldCount() const
{
  return mAttributeVector.size();
}


const QgsFieldMap& QgsGPXProvider::fields() const
{
  return mAttributeFields;
}


bool QgsGPXProvider::isValid()
{
  return mValid;
}


bool QgsGPXProvider::addFeatures( QgsFeatureList & flist )
{
  QMutexLocker locker(&mDataMutex);

  // add all the features
  for ( QgsFeatureList::iterator iter = flist.begin();
        iter != flist.end(); ++iter )
  {
    if ( !addFeature( *iter ) )
      return false;
  }

  // write back to file
  QFile file( mFileName );
  if ( !file.open( QIODevice::WriteOnly ) )
    return false;
  QTextStream ostr( &file );
  data->writeXML( ostr );
  return true;
}


bool QgsGPXProvider::addFeature( QgsFeature& f )
{
  unsigned char* geo = f.geometry()->asWkb();
  QGis::WkbType wkbType = f.geometry()->wkbType();
  bool success = false;
  QgsGPSObject* obj = NULL;
  const QgsAttributeMap& attrs( f.attributeMap() );
  QgsAttributeMap::const_iterator it;

  // is it a waypoint?
  if ( mFeatureType == WaypointType && geo != NULL && wkbType == QGis::WKBPoint )
  {

    // add geometry
    QgsWaypoint wpt;
    std::memcpy( &wpt.lon, geo + 5, sizeof( double ) );
    std::memcpy( &wpt.lat, geo + 13, sizeof( double ) );

    // add waypoint-specific attributes
    for ( it = attrs.begin(); it != attrs.end(); ++it )
    {
      if ( it.key() == EleAttr )
      {
        bool eleIsOK;
        double ele = it->toDouble( &eleIsOK );
        if ( eleIsOK )
          wpt.ele = ele;
      }
      else if ( it.key() == SymAttr )
      {
        wpt.sym = it->toString();
      }
    }

    QgsGPSData::WaypointIterator iter = data->addWaypoint( wpt );
    success = true;
    obj = &( *iter );
  }

  // is it a route?
  if ( mFeatureType == RouteType && geo != NULL && wkbType == QGis::WKBLineString )
  {

    QgsRoute rte;

    // reset bounds
    rte.xMin = std::numeric_limits<double>::max();
    rte.xMax = -std::numeric_limits<double>::max();
    rte.yMin = std::numeric_limits<double>::max();
    rte.yMax = -std::numeric_limits<double>::max();

    // add geometry
    int nPoints;
    std::memcpy( &nPoints, geo + 5, 4 );
    for ( int i = 0; i < nPoints; ++i )
    {
      double lat, lon;
      std::memcpy( &lon, geo + 9 + 16 * i, sizeof( double ) );
      std::memcpy( &lat, geo + 9 + 16 * i + 8, sizeof( double ) );
      QgsRoutepoint rtept;
      rtept.lat = lat;
      rtept.lon = lon;
      rte.points.push_back( rtept );
      rte.xMin = rte.xMin < lon ? rte.xMin : lon;
      rte.xMax = rte.xMax > lon ? rte.xMax : lon;
      rte.yMin = rte.yMin < lat ? rte.yMin : lat;
      rte.yMax = rte.yMax > lat ? rte.yMax : lat;
    }

    // add route-specific attributes
    for ( it = attrs.begin(); it != attrs.end(); ++it )
    {
      if ( it.key() == NumAttr )
      {
        bool numIsOK;
        long num = it->toInt( &numIsOK );
        if ( numIsOK )
          rte.number = num;
      }
    }

    QgsGPSData::RouteIterator iter = data->addRoute( rte );
    success = true;
    obj = &( *iter );
  }

  // is it a track?
  if ( mFeatureType == TrackType && geo != NULL && wkbType == QGis::WKBLineString )
  {

    QgsTrack trk;
    QgsTrackSegment trkseg;

    // reset bounds
    trk.xMin = std::numeric_limits<double>::max();
    trk.xMax = -std::numeric_limits<double>::max();
    trk.yMin = std::numeric_limits<double>::max();
    trk.yMax = -std::numeric_limits<double>::max();

    // add geometry
    int nPoints;
    std::memcpy( &nPoints, geo + 5, 4 );
    for ( int i = 0; i < nPoints; ++i )
    {
      double lat, lon;
      std::memcpy( &lon, geo + 9 + 16 * i, sizeof( double ) );
      std::memcpy( &lat, geo + 9 + 16 * i + 8, sizeof( double ) );
      QgsTrackpoint trkpt;
      trkpt.lat = lat;
      trkpt.lon = lon;
      trkseg.points.push_back( trkpt );
      trk.xMin = trk.xMin < lon ? trk.xMin : lon;
      trk.xMax = trk.xMax > lon ? trk.xMax : lon;
      trk.yMin = trk.yMin < lat ? trk.yMin : lat;
      trk.yMax = trk.yMax > lat ? trk.yMax : lat;
    }

    // add track-specific attributes
    for ( it = attrs.begin(); it != attrs.end(); ++it )
    {
      if ( it.key() == NumAttr )
      {
        bool numIsOK;
        long num = it->toInt( &numIsOK );
        if ( numIsOK )
          trk.number = num;
      }
    }

    trk.segments.push_back( trkseg );
    QgsGPSData::TrackIterator iter = data->addTrack( trk );
    success = true;
    obj = &( *iter );
  }


  // add common attributes
  if ( obj )
  {
    for ( it = attrs.begin(); it != attrs.end(); ++it )
    {
      if ( it.key() == NameAttr )
      {
        obj->name = it->toString();
      }
      else if ( it.key() == CmtAttr )
      {
        obj->cmt = it->toString();
      }
      else if ( it.key() == DscAttr )
      {
        obj->desc = it->toString();
      }
      else if ( it.key() == SrcAttr )
      {
        obj->src = it->toString();
      }
      else if ( it.key() == URLAttr )
      {
        obj->url = it->toString();
      }
      else if ( it.key() == URLNameAttr )
      {
        obj->urlname = it->toString();
      }
    }
  }

  return success;
}


bool QgsGPXProvider::deleteFeatures( const QgsFeatureIds & id )
{
  QMutexLocker locker(&mDataMutex);

  if ( mFeatureType == WaypointType )
    data->removeWaypoints( id );
  else if ( mFeatureType == RouteType )
    data->removeRoutes( id );
  else if ( mFeatureType == TrackType )
    data->removeTracks( id );

  // write back to file
  QFile file( mFileName );
  if ( !file.open( QIODevice::WriteOnly ) )
    return false;
  QTextStream ostr( &file );
  data->writeXML( ostr );
  return true;
}


bool QgsGPXProvider::changeAttributeValues( const QgsChangedAttributesMap & attr_map )
{
  QMutexLocker locker(&mDataMutex);

  QgsChangedAttributesMap::const_iterator aIter = attr_map.begin();
  if ( mFeatureType == WaypointType )
  {
    QgsGPSData::WaypointIterator wIter = data->waypointsBegin();
    for ( ; wIter != data->waypointsEnd() && aIter != attr_map.end(); ++wIter )
    {
      if ( wIter->id == aIter.key() )
      {
        changeAttributeValues( *wIter, aIter.value() );
        ++aIter;
      }
    }
  }
  else if ( mFeatureType == RouteType )
  {
    QgsGPSData::RouteIterator rIter = data->routesBegin();
    for ( ; rIter != data->routesEnd() && aIter != attr_map.end(); ++rIter )
    {
      if ( rIter->id == aIter.key() )
      {
        changeAttributeValues( *rIter, aIter.value() );
        ++aIter;
      }
    }
  }
  if ( mFeatureType == TrackType )
  {
    QgsGPSData::TrackIterator tIter = data->tracksBegin();
    for ( ; tIter != data->tracksEnd() && aIter != attr_map.end(); ++tIter )
    {
      if ( tIter->id == aIter.key() )
      {
        changeAttributeValues( *tIter, aIter.value() );
        ++aIter;
      }
    }
  }

  // write back to file
  QFile file( mFileName );
  if ( !file.open( QIODevice::WriteOnly ) )
    return false;
  QTextStream ostr( &file );
  data->writeXML( ostr );
  return true;
}


void QgsGPXProvider::changeAttributeValues( QgsGPSObject& obj, const QgsAttributeMap& attrs )
{
  QgsAttributeMap::const_iterator aIter;

  // TODO:
  if ( attrs.contains( NameAttr ) )
    obj.name = attrs[NameAttr].toString();
  if ( attrs.contains( CmtAttr ) )
    obj.cmt = attrs[CmtAttr].toString();
  if ( attrs.contains( DscAttr ) )
    obj.desc = attrs[DscAttr].toString();
  if ( attrs.contains( SrcAttr ) )
    obj.src = attrs[SrcAttr].toString();
  if ( attrs.contains( URLAttr ) )
    obj.url = attrs[URLAttr].toString();
  if ( attrs.contains( URLNameAttr ) )
    obj.urlname = attrs[URLNameAttr].toString();

  // waypoint-specific attributes
  QgsWaypoint* wpt = dynamic_cast<QgsWaypoint*>( &obj );
  if ( wpt != NULL )
  {
    if ( attrs.contains( SymAttr ) )
      wpt->sym = attrs[SymAttr].toString();
    if ( attrs.contains( EleAttr ) )
    {
      bool eleIsOK;
      double ele = attrs[EleAttr].toDouble( &eleIsOK );
      if ( eleIsOK )
        wpt->ele = ele;
    }
  }

  // route- and track-specific attributes
  QgsGPSExtended* ext = dynamic_cast<QgsGPSExtended*>( &obj );
  if ( ext != NULL )
  {
    if ( attrs.contains( NumAttr ) )
    {
      bool eleIsOK;
      double ele = attrs[NumAttr].toDouble( &eleIsOK );
      if ( eleIsOK )
        wpt->ele = ele;
    }
  }
}


QVariant QgsGPXProvider::defaultValue( int fieldId )
{
  if ( fieldId == SrcAttr )
    return tr( "Digitized in QGIS" );
  return QVariant();
}



QString QgsGPXProvider::name() const
{
  return GPX_KEY;
} // QgsGPXProvider::name()



QString QgsGPXProvider::description() const
{
  return GPX_DESCRIPTION;
} // QgsGPXProvider::description()

QgsCoordinateReferenceSystem QgsGPXProvider::crs()
{
  return QgsCoordinateReferenceSystem( GEOSRID, QgsCoordinateReferenceSystem::PostgisCrsId ); // use WGS84
}






/**
 * Class factory to return a pointer to a newly created
 * QgsGPXProvider object
 */
QGISEXTERN QgsGPXProvider * classFactory( const QString *uri )
{
  return new QgsGPXProvider( *uri );
}


/** Required key function (used to map the plugin to a data store type)
*/
QGISEXTERN QString providerKey()
{
  return GPX_KEY;
}


/**
 * Required description function
 */
QGISEXTERN QString description()
{
  return GPX_DESCRIPTION;
}


/**
 * Required isProvider function. Used to determine if this shared library
 * is a data provider plugin
 */
QGISEXTERN bool isProvider()
{
  return true;
}


