/***************************************************************************
                          qgsoptions.h
                        Set user options and preferences
                             -------------------
    begin                : May 28, 2004
    copyright            : (C) 2004 by Gary E.Sherman
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
#ifndef QGSOPTIONS_H
#define QGSOPTIONS_H

#include "ui_qgsoptionsbase.h"
#include "qgisgui.h"
#include "qgscontexthelp.h"


/**
 * \class QgsOptions
 * \brief Set user options and preferences
 */
class QgsOptions : public QDialog, private Ui::QgsOptionsBase
{
    Q_OBJECT
  public:
    /**
     * Constructor
     * @param parent Parent widget (usually a QgisApp)
     * @param name name for the widget
     * @param modal true for modal dialog
     */
    QgsOptions( QWidget *parent = 0, Qt::WFlags fl = QgisGui::ModalDialogFlags );
    //! Destructor
    ~QgsOptions();
    /**
     * Return the currently selected theme
     * @return theme name (a directory name in the themes directory)
     */
    QString theme();

  public slots:
    //! Slot called when user chooses to change the project wide projection.
    void on_pbnSelectProjection_clicked();
    void on_chkAntiAliasing_stateChanged();
    void on_chkUseQPixmap_stateChanged();
    void saveOptions();
    //! Slot to change the theme this is handled when the user
    // activates or highlights a theme name in the drop-down list
    void themeChanged( const QString & );

    /**
     * Return the desired state of newly added layers. If a layer
     * is to be drawn when added to the map, this function returns
     * true.
     */
    bool newVisible();
    /*!
     * Slot to select the default map selection color
     */
    void on_pbnSelectionColor_clicked();

    /*!
     * Slot to select the default measure tool color
     */
    void on_pbnMeasureColor_clicked();

    /*!
     * Slot to select the default map selection color
     */
    void on_pbnCanvasColor_clicked();

    /*!
     * Slot to select the color of the digitizing rubber band
     */
    void on_mLineColorToolButton_clicked();

    /**Add a new URL to exclude from Proxy*/
    void on_mAddUrlPushButton_clicked();

    /**Remove an URL to exclude from Proxy*/
    void on_mRemoveUrlPushButton_clicked();

    /* Let the user add a path to the list of search paths
     * used for finding SVG files.
     * @note added in QGIS 1.4
     */
    void on_mBtnAddSVGPath_clicked();

    /* Let the user remove a path to the list of search paths
     * used for finding SVG files.
     * @note added in QGIS 1.4
     */
    void on_mBtnRemoveSVGPath_clicked();

    void on_buttonBox_helpRequested() { QgsContextHelp::run( metaObject()->className() ); }

    void on_mBrowseCacheDirectory_clicked();
    void on_mClearCache_clicked();

  protected:
    //! Populates combo box with ellipsoids
    void getEllipsoidList();

    QString getEllipsoidAcronym( QString theEllipsoidName );
    QString getEllipsoidName( QString theEllipsoidAcronym );

  private:
    //
    QStringList i18nList();

    //!Default proj4 string used for new layers added that have no projection
    QString mGlobalProj4String;

};

#endif // #ifndef QGSOPTIONS_H
