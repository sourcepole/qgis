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

#ifndef QGSPLUGINLAYERREGSITRY_H
#define QGSPLUGINLAYERREGSITRY_H

#include <map>

#include <QDomNode>

class QgsMapLayer;

/** \ingroup core
    callback class for creating plugin specific layers
*/
class CORE_EXPORT QgsPluginLayerCreator
{
  public:

    QgsPluginLayerCreator();
    virtual ~QgsPluginLayerCreator();

    /** return new layer if node data corresponds to the parent plugin, else return NULL */
    virtual QgsMapLayer* createLayer(const QDomNode& layerNode);
};

//=============================================================================

/** \ingroup core
    a registry of callbacks to create plugin layers
*/
class CORE_EXPORT QgsPluginLayerRegistry
{
  public:

    /** means of accessing canonical single instance  */
    static QgsPluginLayerRegistry* instance();

    ~QgsPluginLayerRegistry();

    /** add plugin layer creator and return unique id */
    unsigned int addCreator(QgsPluginLayerCreator* creator);
    /** remove plugin layer creator with id */
    void removeCreator(unsigned int id);

    /** return new layer if corresponding plugin has been found, else return NULL */
    QgsMapLayer* createLayer(const QDomNode& layerNode);

  private:

    typedef std::map<unsigned int, QgsPluginLayerCreator*> PluginLayerCreators;

    /** private since instance() creates it */
    QgsPluginLayerRegistry();

    /** pointer to canonical Singleton object */
    static QgsPluginLayerRegistry* _instance;

    PluginLayerCreators mPluginLayerCreators;
    unsigned int mLastId;
};

#endif // QGSPLUGINLAYERREGSITRY_H
