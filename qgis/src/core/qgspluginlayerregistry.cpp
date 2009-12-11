/***************************************************************************
                    qgspluginlayerregistry.cpp - class for
                    registering plugin layer creators
                             -------------------
    begin                : Mon Nov 30 2009
    copyright            : (C) 2009 by Mathias Walker, Sourcepole
    email                : mwa at sourcepole.ch
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

#include "qgspluginlayerregistry.h"
#include "qgslogger.h"
#include "qgsmaplayer.h"

QgsPluginLayerCreator::QgsPluginLayerCreator()
{
}

QgsPluginLayerCreator::~QgsPluginLayerCreator()
{
}

QgsMapLayer* QgsPluginLayerCreator::createLayer(const QDomNode& layerNode)
{
  return NULL;
}

//=============================================================================

/** Static calls to enforce singleton behaviour */
QgsPluginLayerRegistry* QgsPluginLayerRegistry::_instance = NULL;
QgsPluginLayerRegistry* QgsPluginLayerRegistry::instance()
{
  if ( _instance == NULL )
  {
    _instance = new QgsPluginLayerRegistry();
  }
  return _instance;
}


QgsPluginLayerRegistry::QgsPluginLayerRegistry()
{
  mLastId = 0;
}

QgsPluginLayerRegistry::~QgsPluginLayerRegistry()
{
  if ( !mPluginLayerCreators.empty() )
  {
    QgsDebugMsg("QgsPluginLayerRegistry::~QgsPluginLayerRegistry(): creator list not empty");
  }
}

unsigned int QgsPluginLayerRegistry::addCreator(QgsPluginLayerCreator* creator)
{
  mLastId++;
  mPluginLayerCreators[mLastId] = creator;
  return mLastId;
}

void QgsPluginLayerRegistry::removeCreator(unsigned int id)
{
  PluginLayerCreators::iterator iter = mPluginLayerCreators.find(id);
  if ( iter != mPluginLayerCreators.end() )
  {
    mPluginLayerCreators.erase(id);
  }
}

QgsMapLayer* QgsPluginLayerRegistry::createLayer(const QDomNode& layerNode)
{
  QgsMapLayer* layer = NULL;
  for ( PluginLayerCreators::iterator iter = mPluginLayerCreators.begin(); iter != mPluginLayerCreators.end(); ++iter )
  {
    layer = iter->second->createLayer(layerNode);
    if ( layer != NULL )
    {
      break;
    }
  }
  return layer;
}
