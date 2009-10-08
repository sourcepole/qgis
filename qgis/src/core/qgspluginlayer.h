/***************************************************************************
                          qgspluginlayer.h  -  description
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
/* $Id$ */

#ifndef QGSPLUGINLAYER_H
#define QGSPLUGINLAYER_H

#include "qgis.h"
#include "qgsmaplayer.h"

#include <QDomNode>

/** \ingroup core
 * This class provides QGIS with the ability to render layers using QGIS plugins.
 *
 * Plugin layers will be saved and loaded in projects with their corresponding plugin parameters.
 *
 * Sample usage of the QgsPluginLayer class:
 *
 * TODO
 *
 */
class CORE_EXPORT QgsPluginLayer : public QgsMapLayer
{
    Q_OBJECT

  public:

    /** Constructor */
    QgsPluginLayer( QString pluginId = QString::null, QString lyrname = QString::null, QString source = QString::null );

    /** Destructor */
    virtual ~QgsPluginLayer();

    /** \brief This is called when the view on the layer needs to be redrawn */
    virtual bool draw( QgsRenderContext& rendererContext );

    /** True if the layer can be edited */
    virtual bool isEditable() const { return false; };

    /** Copies the symbology settings from another layer. Returns true in case of success */
    virtual bool copySymbologySettings( const QgsMapLayer& ) { return false; };

    /** Returns true if this layer can be in the same symbology group with another layer */
    virtual bool hasCompatibleSymbology( const QgsMapLayer& ) const { return false; };

    /** \brief Read the symbology for the current layer from the Dom node supplied */
    virtual bool readSymbology( const QDomNode&, QString& ) { return true; };

    /** \brief Write the symbology for the layer into the docment provided */
    virtual bool writeSymbology( QDomNode&, QDomDocument&, QString& ) const { return true; };

    /**
     * Get plugin ID
     */
    QString pluginId();

    /**
     * Set plugin ID
     */
    void setPluginId( const QString& pluginId );

    /**
     * Get plugin specific layer properties from project
     */
    const QDomNode& pluginProperties();

    /**
     * Set layer extent
     */
    void setExtent( const QgsRectangle& extent );

  signals:

    //! emitted when layer is deleted
    void layerDeleted();

    //! emitted when drawing layer
    void drawLayer( QgsRenderContext& );

    //! emitted when saving layer
    void writePluginProperties( QDomNode& propertiesNode, QDomDocument& doc );

  protected:

    /** \brief Reads layer specific state from project file Dom node */
    bool readXml( QDomNode& layerNode );

    /** \brief Write layer specific state to project file Dom node */
    bool writeXml( QDomNode& layerNode, QDomDocument& doc );

  private:

    QString mPluginId;
    QDomNode mPluginProperties;
};

#endif
