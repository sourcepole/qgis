/***************************************************************************
                          qgsvectorlayer.h  -  description
                             -------------------
    begin                : Oct 29, 2003
    copyright            : (C) 2003 by Gary E.Sherman
    email                : sherman at mrcc.com
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

/** \ingroup core
 * Layer rendered by a plugin.
 */
class CORE_EXPORT QgsPluginLayer : public QgsMapLayer
{
    Q_OBJECT

public:
       /** Constructor */
    QgsPluginLayer( QString lyrname = QString::null, QString source = QString::null );

    /** Destructor */
    virtual ~QgsPluginLayer();

    /** True if the layer can be edited */
    virtual bool isEditable() const { return false; };

    /** Copies the symbology settings from another layer. Returns true in case of success */
    virtual bool copySymbologySettings( const QgsMapLayer& ) { return false; };

    /** Returns true if this layer can be in the same symbology group with another layer */
    virtual bool hasCompatibleSymbology( const QgsMapLayer& ) const { return false; };

    /** Read the symbology for the current layer from the Dom node supplied.
     * @param QDomNode node that will contain the symbology definition for this layer.
     * @param errorMessage reference to string that will be updated with any error messages
     * @return true in case of success.
    */
    virtual bool readSymbology( const QDomNode&, QString& ) { return true; };

    /** Write the symbology for the layer into the docment provided.
     *  @param QDomNode the node that will have the style element added to it.
     *  @param QDomDocument the document that will have the QDomNode added.
     * @param errorMessage reference to string that will be updated with any error messages
     *  @return true in case of success.
     */
    virtual bool writeSymbology( QDomNode&, QDomDocument&, QString& ) const { return true; };

  signals:

    void layerDeleted();
};

#endif
