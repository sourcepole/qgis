/***************************************************************************
    qgsmapcanvasmap.cpp  -  draws the map in map canvas
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

#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvasmap.h"
#include "qgsmaprenderer.h"

#include <QPainter>

QgsMapCanvasMap::QgsMapCanvasMap( QgsMapCanvas* canvas )
    : mCanvas( canvas )
{
  setZValue( -10 );
  setPos( 0, 0 );
  resize( QSize( 1, 1 ) );
  mUseQImageToRender = false;

  mDirty = true;

  connect( mCanvas->mapRenderer(), SIGNAL(finishedThreadedRendering(QImage)), SLOT(renderingFinished(QImage)));
  connect( &mMapUpdateTimer, SIGNAL(timeout()), this, SLOT(updateMap()));
}

void QgsMapCanvasMap::paint( QPainter* p, const QStyleOptionGraphicsItem*, QWidget* )
{

  if ( mDirty )
  {
    if ( mCanvas->isDrawing() )
    {
      QgsDebugMsg("drawing already started");
    }
    else if ( ! mCanvas->renderFlag() || mCanvas->isFrozen() )
    {
      QgsDebugMsg("redraw ignored: canvas frozen");
    }
    else
    {
      emit renderStarting();

      // TRIGGER RENDERING
      qDebug("STARTING \n\n\n\n\n");
      mCanvas->mapRenderer()->startThreadedRendering();

      mMapUpdateTimer.start(250);

      updateMap();
    }
  }
  else
  {
    QgsDebugMsg("not dirty");
  }

  //refreshes the canvas map with the current offscreen image
  p->drawPixmap( 0, 0, mPixmap );
}

void QgsMapCanvasMap::updateMap()
{
  QgsDebugMsg("updating map!");
  QImage i = mCanvas->mapRenderer()->threadedRenderingOutput();
  if (!i.isNull())
  {
    setMap(i);
    update();
  }
}

void QgsMapCanvasMap::renderingFinished(QImage img)
{
  QgsDebugMsg("finished!!!");

  mMapUpdateTimer.stop();

  mDirty = false;

  // inform canvas
  mCanvas->renderingFinished(img);

  setMap(img);
  update();
}

void QgsMapCanvasMap::cancelRendering()
{
  if ( mCanvas->isDrawing() )
  {
    mCanvas->mapRenderer()->cancelThreadedRendering();
  }
}


QRectF QgsMapCanvasMap::boundingRect() const
{
  return QRectF( 0, 0, mPixmap.width(), mPixmap.height() );
}


void QgsMapCanvasMap::resize( QSize size )
{
  prepareGeometryChange(); // to keep QGraphicsScene indexes up to date on size change

  mPixmap = QPixmap( size );
  mPixmap.fill(mBgColor.rgb());
  //mImage = QImage( size, QImage::Format_RGB32 ); // temporary image - build it here so it is available when switching from QPixmap to QImage rendering
  mCanvas->mapRenderer()->setOutputSize( size, mPixmap.logicalDpiX() );
}

void QgsMapCanvasMap::setPanningOffset( const QPoint& point )
{
  mOffset = point;
  setPos( mOffset );
}

void QgsMapCanvasMap::render()
{
  /*
  // Rendering to a QImage gives incorrectly filled polygons in some
  // cases (as at Qt4.1.4), but it is the only renderer that supports
  // anti-aliasing, so we provide the means to swap between QImage and
  // QPixmap.

  if ( mUseQImageToRender )
  {
    // use temporary image for rendering
    mImage.fill( mBgColor.rgb() );

    // clear the pixmap so that old map won't be displayed while rendering
    // TODO: do the canvas updates wisely -> this wouldn't be needed
    mPixmap = QPixmap( mImage.size() );
    mPixmap.fill( mBgColor.rgb() );

    QPainter paint;
    paint.begin( &mImage );
    // Clip drawing to the QImage
    paint.setClipRect( mImage.rect() );

    // antialiasing
    if ( mAntiAliasing )
      paint.setRenderHint( QPainter::Antialiasing );

    mCanvas->mapRenderer()->render( &paint );

    paint.end();

    // convert QImage to QPixmap to acheive faster drawing on screen
    mPixmap = QPixmap::fromImage( mImage );
  }
  else
  {
    mPixmap.fill( mBgColor.rgb() );
    QPainter paint;
    paint.begin( &mPixmap );
    // Clip our drawing to the QPixmap
    paint.setClipRect( mPixmap.rect() );
    mCanvas->mapRenderer()->render( &paint );
    paint.end();
  }
  update();
  */
}

QPaintDevice& QgsMapCanvasMap::paintDevice()
{
  return mPixmap;
}

void QgsMapCanvasMap::updateContents()
{
  /*
  // make sure we're using current contents
  if ( mUseQImageToRender )
    mPixmap = QPixmap::fromImage( mImage );

  // trigger update of this item
  update();
  */
}
