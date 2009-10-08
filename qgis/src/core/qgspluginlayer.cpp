/***************************************************************************
                               qgspluginlayer.cpp
  This class implements a generic means to render layers using QGIS plugins.
                              -------------------
          begin                : Sep 30, 2009
          copyright            : (C) 2009 by Sourcepole
          email                : info at sourcepole.ch

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

#include "qgspluginlayer.h"

#include <QDomElement>

#include "qgslogger.h"

QgsPluginLayer::QgsPluginLayer( QString pluginId, QString lyrname, QString source )
    : QgsMapLayer( PluginLayer, lyrname, source )
{
  mPluginId = pluginId;
  mValid = true;
}

QgsPluginLayer::~QgsPluginLayer()
{
  emit layerDeleted();
}

bool QgsPluginLayer::draw( QgsRenderContext& rendererContext )
{
  emit drawLayer(rendererContext);
  return true;
}

QString QgsPluginLayer::pluginId()
{
  return mPluginId;
}

void QgsPluginLayer::setPluginId( const QString& pluginId )
{
  mPluginId = pluginId;
}

const QDomNode& QgsPluginLayer::pluginProperties()
{
  return mPluginProperties;
}

void QgsPluginLayer::setExtent( const QgsRectangle& extent )
{
  mLayerExtent = extent;
}

/**
  Plugin layer project file XML of form:

  <maplayer minimumScale="0" maximumScale="1e+08" type="plugin" hasScaleBasedVisibilityFlag="0" >
    <id>Mapfile_0123456789</id>
    <datasource>./mapfile.map</datasource>
    <layername>Mapfile</layername>
    <srs>...</srs>
    <transparencyLevelInt>255</transparencyLevelInt>
    <pluginId>MapfileLayer</pluginId>
    <pluginproperties>
      // plugin specific values
    </pluginproperties>
  </maplayer>

  @note Called by QgsMapLayer::readXML().
*/
bool QgsPluginLayer::readXml( QDomNode& layerNode )
{
  QDomNode pluginIdNode = layerNode.namedItem("pluginId");
  if ( pluginIdNode.isNull() )
  {
    mPluginId = "";
  }
  else
  {
    QDomElement pluginIdEl = pluginIdNode.toElement();
    mPluginId = pluginIdEl.text();
  }

  mPluginProperties = layerNode.namedItem("pluginproperties");

  return true;
}

/*
 *  virtual
 *  @note Called by QgsMapLayer::writeXML().
 */
bool QgsPluginLayer::writeXml( QDomNode& layerNode, QDomDocument& document )
{
  QDomElement mapLayerNode = layerNode.toElement();
  if ( mapLayerNode.isNull() || ( "maplayer" != mapLayerNode.nodeName() ) )
  {
    QgsLogger::warning( "QgsPluginLayer::writeXML() can't find <maplayer>" );
    return false;
  }

  mapLayerNode.setAttribute( "type", "plugin" );

  // add plugin id
  QDomElement pluginId  = document.createElement( "pluginId" );
  QDomText pluginIdText = document.createTextNode( mPluginId );
  pluginId.appendChild( pluginIdText );
  layerNode.appendChild( pluginId );

  // add plugin properties
  QDomElement properties  = document.createElement( "pluginproperties" );
  layerNode.appendChild( properties );

  emit writePluginProperties( properties, document );

  return true;
}
