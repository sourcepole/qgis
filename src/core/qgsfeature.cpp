/***************************************************************************
                qgsfeature.cpp - Spatial Feature Implementation
                     --------------------------------------
Date                 : 09-Sep-2003
Copyright            : (C) 2003 by Gary E.Sherman
email                : sherman at mrcc.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgsrectangle.h"


QgsFeatureData::QgsFeatureData( int id )
    : mFid( id ),
    mGeometry( 0 ),
    mOwnsGeometry( 0 ),
    mValid( false ),
    mAttributeMapDirty( false )
{}

QgsFeatureData::QgsFeatureData( const QgsFeatureData & rhs )
    : QSharedData( rhs ),
    mFid( rhs.mFid ),
    mAttributes( rhs.mAttributes ),
    mAttributeVector( rhs.mAttributeVector ),
    mGeometry( 0 ),
    mOwnsGeometry( false ),
    mValid( rhs.mValid ),
    mAttributeMapDirty( rhs.mAttributeMapDirty )
{
  // copy embedded geometry
  if ( rhs.mGeometry )
  {
    setGeometry( new QgsGeometry( *rhs.mGeometry ) );
  }
}

QgsFeatureData::~QgsFeatureData()
{
  // Destruct the attached geometry only if we still own it.
  if ( mOwnsGeometry && mGeometry )
    delete mGeometry;
}

void QgsFeatureData::setGeometry( QgsGeometry* geom )
{
  // Destruct the attached geometry only if we still own it, before assigning new one.
  if ( mOwnsGeometry && mGeometry )
  {
    delete mGeometry;
    mGeometry = 0;
  }

  mGeometry = geom;
  mOwnsGeometry = true;
}

void QgsFeatureData::clearAttributes()
{
  mAttributeVector.clear();
  //qFill(d->mAttributeVector, QVariant() );
  mAttributeMapDirty = true;
}

void QgsFeatureData::setAttributes( const QgsAttributeMap& attributes )
{
  mAttributes = attributes;
  mAttributeMapDirty = false;

  // update attribute vector from the map
  convertMapToVector();
}

void QgsFeatureData::convertMapToVector()
{
  int max_key = 0;
  QgsAttributeMap::const_iterator it;
  for ( it = mAttributes.constBegin(); it != mAttributes.constEnd(); ++it )
  {
    if ( it.key() > max_key )
      max_key = it.key();
  }

  mAttributeVector.resize( max_key + 1 );
  QVariant* data = mAttributeVector.data();

  for ( it = mAttributes.constBegin(); it != mAttributes.constEnd(); ++it )
  {
    data[ it.key()] = it.value();
  }
}

void QgsFeatureData::convertVectorToMap() const
{
  int count = mAttributeVector.size();
  const QVariant* data = mAttributeVector.constData();

  mAttributes.clear();
  for ( int i = 0; i < count; i++ )
  {
    mAttributes.insert( i, data[i] );
  }

  mAttributeMapDirty = false;
}

void QgsFeatureData::setAttributes( const QgsAttributeVector& attributes )
{
  mAttributeVector = attributes;
  mAttributeMapDirty = true;
}

void QgsFeatureData::addAttribute( int field, QVariant attr )
{
  //if (field >= mAttributeVector.count())
  //  mAttributeVector.resize(field+1);
  mAttributeVector.insert( field, attr );

  mAttributeMapDirty = true;
}

void QgsFeatureData::deleteAttribute( int field )
{
  mAttributeVector.remove( field );
  mAttributeMapDirty = true;
}

void QgsFeatureData::changeAttribute( int field, QVariant attr )
{
  mAttributeVector[field] = attr;
  mAttributeMapDirty = true;
}



////////

QgsFeature::QgsFeature( int id, QString )
{
  d = new QgsFeatureData( id );
}

QgsFeature::QgsFeature( QgsFeature const & rhs )
    : d( rhs.d )
{
}

QgsFeature::~QgsFeature()
{

}

int QgsFeature::id() const
{
  return d->mFid;
}

void QgsFeature::setFeatureId( int id )
{
  d->mFid = id;
}

const QgsAttributeMap& QgsFeature::attributeMap() const
{
  if ( d->mAttributeMapDirty )
  {
    d->convertVectorToMap();
  }
  return d->mAttributes;
}

void QgsFeature::setAttributeMap( const QgsAttributeMap& attributes )
{
  d->setAttributes( attributes );
}

void QgsFeature::clearAttributeMap()
{
  d->clearAttributes();
}

void QgsFeature::addAttribute( int field, QVariant attr )
{
  d->addAttribute( field, attr );
}

void QgsFeature::deleteAttribute( int field )
{
  d->deleteAttribute( field );
}

void QgsFeature::changeAttribute( int field, QVariant attr )
{
  d->changeAttribute( field, attr );
}

QgsGeometry *QgsFeature::geometry()
{
  return d->mGeometry;
}

QgsGeometry *QgsFeature::geometryAndOwnership()
{
  d->mOwnsGeometry = false;

  return d->mGeometry;
}

QString QgsFeature::typeName() const
{
  return QString();
}

void QgsFeature::setTypeName( QString typeName )
{
}

void QgsFeature::setGeometry( const QgsGeometry& geom )
{
  d->setGeometry( new QgsGeometry( geom ) );
}

void QgsFeature::setGeometry( QgsGeometry* geom )
{
  d->setGeometry( geom );
}

void QgsFeature::setGeometryAndOwnership( unsigned char *geom, size_t length )
{
  QgsGeometry *g = new QgsGeometry();
  g->fromWkb( geom, length );
  d->setGeometry( g );
}


bool QgsFeature::isValid() const
{
  return d->mValid;
}

void QgsFeature::setValid( bool validity )
{
  d->mValid = validity;
}

bool QgsFeature::isDirty() const
{
  return false;
}

void QgsFeature::clean()
{
}

QVariant* QgsFeature::resizeAttributeVector( int fieldCount )
{
  d->mAttributeVector.resize( fieldCount );
  d->mAttributeMapDirty = true;
  return d->mAttributeVector.data();
}

void QgsFeature::setAttributeVector( const QgsAttributeVector& attrs )
{
  d->setAttributes( attrs );
}

const QgsAttributeVector& QgsFeature::attributeVector() const
{
  return d->mAttributeVector;
}
