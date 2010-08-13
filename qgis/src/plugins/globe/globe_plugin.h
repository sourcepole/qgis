/***************************************************************************
    globe.h

    Globe Plugin
    a QGIS plugin
     --------------------------------------
    Date                 : 08-Jul-2010
    Copyright            : (C) 2010 by Sourcepole
    Email                : info at sourcepole.ch
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS_GLOBE_PLUGIN_H
#define QGS_GLOBE_PLUGIN_H

#include "../qgisplugin.h"
#include "qgsosgviewer.h"
#include <QObject>
#include <QDockWidget>

class QAction;
class QToolBar;
class QgisInterface;

class GlobePlugin : public QObject, public QgisPlugin
{
  Q_OBJECT

  public:
    GlobePlugin( QgisInterface* theQgisInterface );
    virtual ~GlobePlugin();

  public slots:
    //! init the gui
    virtual void initGui();
    //! Show the dialog box
    void run();
    //! unload the plugin
    void unload();
    //! show the help document
    void help();

  private:
    int mPluginType;
    //! Pointer to the QGIS interface object
    QgisInterface *mQGisIface;
    //!pointer to the qaction for this plugin
    QAction * mQActionPointer;
    //! OSG Viewer
    QgsOsgViewer viewer;
    //! Dock widget for viewer
    QDockWidget mQDockWidget;
};

#endif // QGS_GLOBE_PLUGIN_H
