/***************************************************************************
     QgsAttributeTableMemoryModel.cpp
     --------------------------------------
    Date                 : Feb 2009
    Copyright            : (C) 2009 Vita Cizek
    Email                : weetya (at) gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsattributetablememorymodel.h"
#include "qgsattributetablefiltermodel.h"

#include "qgsfield.h"
#include "qgsvectorlayer.h"
#include "qgslogger.h"

#include <QtGui>
#include <QVariant>

/////////////////////
// In-Memory model //
/////////////////////

void QgsAttributeTableMemoryModel::loadLayer()
{
  QgsAttributeTableModel::loadLayer();
  QgsFeatureIterator fi = mLayer->getFeatures( mLayer->pendingAllAttributesList(), QgsRectangle(), false );

  mFeatureMap.reserve( mLayer->pendingFeatureCount() + 50 );

  QgsFeature f;
  while ( fi.nextFeature( f ) )
    mFeatureMap.insert( f.id(), f );
}

QgsAttributeTableMemoryModel::QgsAttributeTableMemoryModel
( QgsVectorLayer *theLayer )
    : QgsAttributeTableModel( theLayer )
{
  // do not call virtual methods in constructors!
  //loadLayer();
}

bool QgsAttributeTableMemoryModel::featureAtId( int fid ) const
{
  if ( mFeatureMap.contains( fid ) )
  {
    mFeat = mFeatureMap[ fid ];
    return true;
  }
  else
  {
    QgsDebugMsg( QString( "feature %1 not loaded" ).arg( fid ) );
    return false;
  }
}

#if 0
void QgsAttributeTableMemoryModel::featureDeleted( int fid )
{
  QgsDebugMsg( "entered." );
  mFeatureMap.remove( fid );
  QgsAttributeTableModel::featureDeleted( fid );
}

void QgsAttributeTableMemoryModel::featureAdded( int fid )
{
  QgsDebugMsg( "entered." );
  QgsFeature f;
  mLayer->featureAtId( fid, f, false, true );
  mFeatureMap.insert( fid, f );
  QgsAttributeTableModel::featureAdded( fid );
}
#endif

void QgsAttributeTableMemoryModel::layerDeleted()
{
  QgsDebugMsg( "entered." );
  mFeatureMap.clear();
  QgsAttributeTableModel::layerDeleted();
}

void QgsAttributeTableMemoryModel::attributeValueChanged( int fid, int idx, const QVariant &value )
{
  QgsDebugMsg( "entered." );
  mFeatureMap[fid].changeAttribute( idx, value );
  reload( index( 0, 0 ), index( rowCount(), columnCount() ) );
}
