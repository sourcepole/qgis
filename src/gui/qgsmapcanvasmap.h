/***************************************************************************
    qgsmapcanvasmap.h  -  draws the map in map canvas
    ----------------------
    begin                : February 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#ifndef QGSMAPCANVASMAP_H
#define QGSMAPCANVASMAP_H

#include <QGraphicsRectItem>
#include <QPixmap>
#include <QTimer>

class QgsMapRenderer;
class QgsMapCanvas;

/** \ingroup gui
 * A rectangular graphics item representing the map on the canvas.
 */
class GUI_EXPORT QgsMapCanvasMap : public QObject, public QGraphicsRectItem
{
  Q_OBJECT

  public:

    //! constructor
    QgsMapCanvasMap( QgsMapCanvas* canvas );

    //! resize canvas item and pixmap
    void resize( QSize size );

    void enableAntiAliasing( bool flag ) { mAntiAliasing = flag; }

    void useImageToRender( bool flag ) { mUseQImageToRender = flag; }

    //! renders map using QgsMapRenderer to mPixmap
    //! @deprecated does nothing
    void render();

    void setBackgroundColor( const QColor& color ) { mBgColor = color; }

    void setPanningOffset( const QPoint& point );

    //deprecated. Please use paintDevice() function
    //which is also save in case QImage is used
    QPixmap& pixmap() { return mPixmap; }

    QPaintDevice& paintDevice();

    void paint( QPainter* p, const QStyleOptionGraphicsItem*, QWidget* );

    QRectF boundingRect() const;

    //! Update contents - can be called while drawing to show the status.
    //! Added in version 1.2
    //! @deprecated does nothing
    void updateContents();

    void setMap(QImage img) { mPixmap = QPixmap::fromImage(img); }

    bool isDirty() const { return mDirty; }
    void setDirty(bool dirty) { mDirty = dirty; }

    //! force stop of the rendering process
    void cancelRendering();

  public slots:
    void updateMap();

    //! Called when asynchronous rendering is finished
    void renderingFinished(QImage img);

  signals:
    void renderStarting();

  private:

    //! indicates whether antialiasing will be used for rendering
    bool mAntiAliasing;

    //! Whether to use a QPixmap or a QImage for the rendering
    bool mUseQImageToRender;

    QPixmap mPixmap;
    //QImage mImage;

    //QgsMapRenderer* mRender;
    QgsMapCanvas* mCanvas;

    QColor mBgColor;

    QPoint mOffset;

    QTimer mMapUpdateTimer;

    /*! \brief Flag to track the state of the Map canvas.
     *
     * The canvas is
     * flagged as dirty by any operation that changes the state of
     * the layers or the view extent. If the canvas is not dirty, paint
     * events are handled by bit-blitting the stored canvas bitmap to
     * the canvas. This improves performance by not reading the data source
     * when no real change has occurred
     */
    bool mDirty;

};

#endif
