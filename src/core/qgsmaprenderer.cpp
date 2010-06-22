/***************************************************************************
    qgsmaprender.cpp  -  class for rendering map layer set
    ----------------------
    begin                : January 2006
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

#include <cmath>
#include <cfloat>

#include "qgscoordinatetransform.h"
#include "qgslogger.h"
#include "qgsmaprenderer.h"
#include "qgsscalecalculator.h"
#include "qgsmaptopixel.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsdistancearea.h"
#include "qgscentralpointpositionmanager.h"
#include "qgsoverlayobjectpositionmanager.h"
#include "qgspalobjectpositionmanager.h"
#include "qgsvectorlayer.h"
#include "qgsvectoroverlay.h"


#include <QDomDocument>
#include <QDomNode>
#include <QPainter>
#include <QListIterator>
#include <QSettings>
#include <QTime>
#include <QtConcurrentMap>
#include "qgslogger.h"


void _renderLayerThreading( ThreadedRenderContext& tctx );

/////

QgsMapRenderer::QgsMapRenderer()
{
  mScaleCalculator = new QgsScaleCalculator;
  mDistArea = new QgsDistanceArea;

  mDrawing = false;
  mOverview = false;
  mThreadingEnabled = false;
  mCache = NULL;

  // set default map units - we use WGS 84 thus use degrees
  setMapUnits( QGis::Degrees );

  mSize = QSize( 0, 0 );

  mProjectionsEnabled = false;
  mDestCRS = new QgsCoordinateReferenceSystem( GEOCRS_ID, QgsCoordinateReferenceSystem::InternalCrsId ); //WGS 84

  mOutputUnits = QgsMapRenderer::Millimeters;

  mLabelingEngine = NULL;

  //connect(&mFW, SIGNAL(finished()), SLOT(futureFinished()));
}

QgsMapRenderer::~QgsMapRenderer()
{
  if ( mDrawing )
  {
    QgsDebugMsg("canceling the rendering...");
    cancelThreadedRendering();
  }

  delete mScaleCalculator;
  delete mDistArea;
  delete mDestCRS;
  delete mLabelingEngine;
  delete mCache;
}


QgsRectangle QgsMapRenderer::extent() const
{
  return mExtent;
}

void QgsMapRenderer::updateScale()
{
  mScale = mScaleCalculator->calculate( mExtent, mSize.width() );
}

bool QgsMapRenderer::setExtent( const QgsRectangle& extent )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return false; // do not allow changes while rendering
  }

  // Don't allow zooms where the current extent is so small that it
  // can't be accurately represented using a double (which is what
  // currentExtent uses). Excluding 0 avoids a divide by zero and an
  // infinite loop when rendering to a new canvas. Excluding extents
  // greater than 1 avoids doing unnecessary calculations.

  // The scheme is to compare the width against the mean x coordinate
  // (and height against mean y coordinate) and only allow zooms where
  // the ratio indicates that there is more than about 12 significant
  // figures (there are about 16 significant figures in a double).

  if ( extent.width()  > 0 &&
       extent.height() > 0 &&
       extent.width()  < 1 &&
       extent.height() < 1 )
  {
    // Use abs() on the extent to avoid the case where the extent is
    // symmetrical about 0.
    double xMean = ( fabs( extent.xMinimum() ) + fabs( extent.xMaximum() ) ) * 0.5;
    double yMean = ( fabs( extent.yMinimum() ) + fabs( extent.yMaximum() ) ) * 0.5;

    double xRange = extent.width() / xMean;
    double yRange = extent.height() / yMean;

    static const double minProportion = 1e-12;
    if ( xRange < minProportion || yRange < minProportion )
      return false;
  }

  mExtent = extent;
  if ( !extent.isEmpty() )
    adjustExtentToSize();
  return true;
}



void QgsMapRenderer::setOutputSize( QSize size, int dpi )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  mSize = size;
  mScaleCalculator->setDpi( dpi );
  adjustExtentToSize();
}
int QgsMapRenderer::outputDpi()
{
  return mScaleCalculator->dpi();
}
QSize QgsMapRenderer::outputSize()
{
  return mSize;
}

void QgsMapRenderer::adjustExtentToSize()
{
  int myHeight = mSize.height();
  int myWidth = mSize.width();

  QgsMapToPixel newCoordXForm;

  if ( !myWidth || !myHeight )
  {
    mScale = 1;
    newCoordXForm.setParameters( 0, 0, 0, 0 );
    return;
  }

  // calculate the translation and scaling parameters
  // mapUnitsPerPixel = map units per pixel
  double mapUnitsPerPixelY = static_cast<double>( mExtent.height() )
                             / static_cast<double>( myHeight );
  double mapUnitsPerPixelX = static_cast<double>( mExtent.width() )
                             / static_cast<double>( myWidth );
  mMapUnitsPerPixel = mapUnitsPerPixelY > mapUnitsPerPixelX ? mapUnitsPerPixelY : mapUnitsPerPixelX;

  // calculate the actual extent of the mapCanvas
  double dxmin, dxmax, dymin, dymax, whitespace;

  if ( mapUnitsPerPixelY > mapUnitsPerPixelX )
  {
    dymin = mExtent.yMinimum();
    dymax = mExtent.yMaximum();
    whitespace = (( myWidth * mMapUnitsPerPixel ) - mExtent.width() ) * 0.5;
    dxmin = mExtent.xMinimum() - whitespace;
    dxmax = mExtent.xMaximum() + whitespace;
  }
  else
  {
    dxmin = mExtent.xMinimum();
    dxmax = mExtent.xMaximum();
    whitespace = (( myHeight * mMapUnitsPerPixel ) - mExtent.height() ) * 0.5;
    dymin = mExtent.yMinimum() - whitespace;
    dymax = mExtent.yMaximum() + whitespace;
  }

  QgsDebugMsg( QString( "Map units per pixel (x,y) : %1, %2" ).arg( mapUnitsPerPixelX ).arg( mapUnitsPerPixelY ) );
  QgsDebugMsg( QString( "Pixmap dimensions (x,y) : %1, %2" ).arg( myWidth ).arg( myHeight ) );
  QgsDebugMsg( QString( "Extent dimensions (x,y) : %1, %2" ).arg( mExtent.width() ).arg( mExtent.height() ) );
  QgsDebugMsg( mExtent.toString() );

  // update extent
  mExtent.setXMinimum( dxmin );
  mExtent.setXMaximum( dxmax );
  mExtent.setYMinimum( dymin );
  mExtent.setYMaximum( dymax );

  // update the scale
  updateScale();

  QgsDebugMsg( QString( "Scale (assuming meters as map units) = 1:%1" ).arg( mScale ) );

  newCoordXForm.setParameters( mMapUnitsPerPixel, dxmin, dymin, myHeight );
  mRenderContext.setMapToPixel( newCoordXForm );
  mRenderContext.setExtent( mExtent );
}


void QgsMapRenderer::initRendering( QPainter* painter, double deviceDpi )
{
  mDrawing = true;

  QgsDebugMsg( "========== Rendering ==========" );
  QgsDebugMsg( "caching enabled? " + QString::number(mCache != NULL) );

#ifdef QGISDEBUG
  QgsDebugMsg( "Starting to render layer stack." );
  mRenderTime.start();
#endif

  mRenderContext.setDrawEditingInformation( !mOverview );
  mRenderContext.setPainter( painter );
  mRenderContext.setCoordinateTransform( 0 );
  //this flag is only for stopping during the current rendering progress,
  //so must be false at every new render operation
  mRenderContext.setRenderingStopped( false );

  //calculate scale factor
  //use the specified dpi and not those from the paint device
  //because sometimes QPainter units are in a local coord sys (e.g. in case of QGraphicsScene)
  double sceneDpi = mScaleCalculator->dpi();
  double scaleFactor = 1.0;
  if ( mOutputUnits == QgsMapRenderer::Millimeters )
  {
    scaleFactor = sceneDpi / 25.4;
  }
  double rasterScaleFactor = deviceDpi / sceneDpi;

  // initialize render context scaling
  mRenderContext.setRasterScaleFactor( rasterScaleFactor );
  mRenderContext.setScaleFactor( scaleFactor );
  mRenderContext.setRendererScale( mScale );

  mRenderContext.setLabelingEngine( mLabelingEngine );
  if ( mLabelingEngine )
    mLabelingEngine->init( this );

  if (mCache)
  {
    // initialize cache: if the parameters are not the same as the last time,
    // the cached images are removed
    mCache->init(mExtent, mScale, scaleFactor, rasterScaleFactor);
  }

  mOverlayManager = overlayManagerFromSettings();
}

void QgsMapRenderer::finishRendering()
{
  // render labels for vector layers (not using PAL)
  if ( !mOverview )
  {
    renderLabels();
  }

  //find overlay positions and draw the vector overlays
  if ( mOverlayManager )
  {
    mOverlayManager->drawOverlays( mRenderContext, mScaleCalculator->mapUnits() );
    delete mOverlayManager;
    mOverlayManager = NULL;
  }

  // make sure progress bar arrives at 100%!
  emit drawingProgress( 1, 1 );

  if ( mLabelingEngine )
  {
    // set correct extent
    mRenderContext.setExtent( mExtent );
    mRenderContext.setCoordinateTransform( NULL );

    mLabelingEngine->drawLabeling( mRenderContext );
    mLabelingEngine->exit();
  }

  QgsDebugMsg( "Rendering completed in (seconds): " + QString( "%1" ).arg( mRenderTime.elapsed() / 1000.0 ) );

  mDrawing = false;
}

void QgsMapRenderer::render( QPainter* painter )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return;
  }

  if ( mExtent.isEmpty() )
  {
    QgsDebugMsg( "empty extent... not rendering" );
    return;
  }

  QPaintDevice* thePaintDevice = painter->device();
  if ( !thePaintDevice )
  {
    return;
  }

  double deviceDpi = ( thePaintDevice->logicalDpiX() + thePaintDevice->logicalDpiY() ) / 2.0;

  mThreadingEnabled = false;

  initRendering( painter, deviceDpi );

  // do the rendering of all layers
  renderLayers();

  finishRendering();
}


void QgsMapRenderer::renderLayers()
{
  mThreadedJobs.clear();

  // render all layers in the stack, starting at the base
  QListIterator<QString> li( mLayerSet );
  li.toBack();
  while ( li.hasPrevious() )
  {
    if ( mRenderContext.renderingStopped() )
    {
      break;
    }

    QString layerId = li.previous();

    QgsDebugMsg( "Rendering at layer item " + layerId );

    //emit drawingProgress(myRenderCounter++, mLayerSet.size());
    QgsMapLayer *ml = QgsMapLayerRegistry::instance()->mapLayer( layerId );

    if ( !ml )
    {
      QgsDebugMsg( "Layer not found in registry!" );
      return;
    }

    //QgsDebugMsg( "  Layer minscale " + QString( "%1" ).arg( ml->minimumScale() ) );
    //QgsDebugMsg( "  Layer maxscale " + QString( "%1" ).arg( ml->maximumScale() ) );
    //QgsDebugMsg( "  Scale dep. visibility enabled? " + QString( "%1" ).arg( ml->hasScaleBasedVisibility() ) );
    //QgsDebugMsg( "  Input extent: " + ml->extent().toString() );

    if ( ml->hasScaleBasedVisibility() && ( ml->minimumScale() > mScale || ml->maximumScale() < mScale ) && ! mOverview )
    {
      QgsDebugMsg( "Layer not rendered because it is not within the defined "
                   "visibility scale range" );
      return;
    }

    QgsCoordinateTransform* ct = NULL;
    if ( hasCrsTransformEnabled() )
    {
      ct = new QgsCoordinateTransform( ml->srs(), *mDestCRS );
    }
    mRenderContext.setCoordinateTransform( ct );

    //create overlay objects for features within the view extent
    if ( ml->type() == QgsMapLayer::VectorLayer && mOverlayManager )
    {
      QgsVectorLayer* vl = qobject_cast<QgsVectorLayer *>( ml );
      if ( vl )
      {
        mOverlayManager->addOverlaysForLayer( vl, mRenderContext );
      }
    }

    // Force render of layers that are being edited
    // or if there's a labeling engine that needs the layer to register features
    if ( mCache && ml->type() == QgsMapLayer::VectorLayer )
    {
      QgsVectorLayer* vl = qobject_cast<QgsVectorLayer *>( ml );
      if ( vl->isEditable() ||
           ( mRenderContext.labelingEngine() && mRenderContext.labelingEngine()->willUseLayer( vl ) ) )
      {
        mCache->setCacheImage(ml->getLayerID(), QImage());
      }
    }

    //
    // Now do the call to the layer that actually does
    // the rendering work!
    //
    connect( ml, SIGNAL( drawingProgress( int, int ) ), this, SLOT( onDrawingProgress( int, int ) ) );

    if ( !mThreadingEnabled )
      renderLayerNoThreading( ml );
    else
      renderLayerThreading( ml );

    disconnect( ml, SIGNAL( drawingProgress( int, int ) ), this, SLOT( onDrawingProgress( int, int ) ) );
  }

  if ( mThreadingEnabled )
  {
    QgsDebugMsg("STARTING THREADED RENDERING!");

    //QtConcurrent::blockingMap(layers, _renderLayerThreading);

    connect(&mFW, SIGNAL(finished()), SLOT(futureFinished()));

    mFuture = QtConcurrent::map(mThreadedJobs, _renderLayerThreading);
    mFW.setFuture(mFuture);
  }

}

QImage QgsMapRenderer::threadedRenderingOutput()
{
  QImage i( mSize, QImage::Format_ARGB32_Premultiplied );
  i.fill(0);
  QPainter p;
  p.begin(&i);
  foreach (const ThreadedRenderContext& tctx, mThreadedJobs)
  {
    if (tctx.img == 0)
      return QImage(); // destroyed already
    p.drawImage( 0, 0, *tctx.img );
  }
  p.end();
  return i;
}

void QgsMapRenderer::futureFinished()
{
  QgsDebugMsg("THREADED RENDERING FINISHED!");

  QImage i = threadedRenderingOutput();

  // delete temporary painters
  foreach (ThreadedRenderContext tctx, mThreadedJobs)
  {
    delete tctx.ctx.painter();
    delete tctx.img;
    tctx.img = 0;
  }
  mThreadedJobs.clear();

  QgsDebugMsg("THREADED RENDERING DONE!");

  disconnect(&mFW, SIGNAL(finished()), this, SLOT(futureFinished()));

  QPainter painter;
  painter.begin(&i);
  mRenderContext.setPainter(&painter);

  // postprocessing
  finishRendering();

  painter.end();

  emit finishedThreadedRendering(i);
}

void QgsMapRenderer::startThreadedRendering()
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  if ( mExtent.isEmpty() )
  {
    QgsDebugMsg( "empty extent... not rendering" );
    return;
  }

  mThreadingEnabled = true;
  initRendering(NULL, 96); // TODO: what dpi to use?

  renderLayers();
}


void QgsMapRenderer::cancelThreadedRendering()
{
  if (!mDrawing)
  {
    QgsDebugMsg("nothing to cancel.");
    return;
  }

  QgsDebugMsg("canceling render");
  for (int i = 0; i < mThreadedJobs.count(); i++)
  {
    mThreadedJobs[i].ctx.setRenderingStopped(true);
  }

  disconnect(&mFW, SIGNAL(finished()), this, SLOT(futureFinished()));

  mFuture.cancel();
  QTime t;
  t.start();
  mFW.waitForFinished();

  QgsDebugMsg(QString("waited %1 ms").arg(t.elapsed() / 1000.0));

  futureFinished(); // manually trigger the routines for finalization
}


void _renderLayerThreading( ThreadedRenderContext& tctx )
{
  QString layerId = tctx.ml->getLayerID();
  if (tctx.cached)
  {
    QgsDebugMsg("threaded cached (doing nothing): "+layerId);
  }
  else
  {
    QgsDebugMsg("threaded rendering start: "+layerId);
    // TODO: error handling
    tctx.mr->renderLayer( tctx.ml, tctx.ctx );

    if (tctx.mr->mCache)
    {
      // save cache image
      tctx.mr->mCache->setCacheImage( layerId, *tctx.img );
    }

    QgsDebugMsg("threaded rendering end  : "+layerId);
  }
}

void QgsMapRenderer::renderLayerThreading( QgsMapLayer* ml )
{
  ThreadedRenderContext tctx;
  tctx.ml = ml;

  if ( mCache && ! mCache->cacheImage(ml->getLayerID()).isNull() )
  {
    // cached image will be used
    tctx.img = new QImage( mCache->cacheImage(ml->getLayerID()));
    tctx.cached = true;
  }
  else
  {
    // create image
    tctx.img = new QImage( mSize, QImage::Format_ARGB32_Premultiplied );
    tctx.img->fill( 0 );
    tctx.cached = false;
  }

  // create private context
  tctx.mr = this;
  tctx.ctx = mRenderContext;

  QPainter* painter = new QPainter(tctx.img);
  painter->setRenderHint( QPainter::Antialiasing, mAntialiasingEnabled );
  tctx.ctx.setPainter( painter );

  // schedule DRAW to a list
  mThreadedJobs.append(tctx);
}

void QgsMapRenderer::renderLayerNoThreading( QgsMapLayer* ml )
{
  if ( mCache )
  {
    // Store the painter in case we need to swap it out for the
    // cache painter
    QPainter * mypContextPainter = mRenderContext.painter();

    // retrieve cached image for the layer (will be null if not valid)
    QImage cacheImage = mCache->cacheImage(ml->getLayerID());

    if ( cacheImage.isNull() )
    {
      // create cached image
      QgsDebugMsg( "\n\n\nCaching enabled --- redraw forced by extent change or empty cache\n\n\n" );
      QPaintDevice* device = mRenderContext.painter()->device();
      cacheImage = QImage( device->width(), device->height(), QImage::Format_ARGB32_Premultiplied );
      cacheImage.fill( 0 );

      // alter painter
      QPainter * mypPainter = new QPainter( &cacheImage );
      mypPainter->setRenderHint( QPainter::Antialiasing, mAntialiasingEnabled );
      mRenderContext.setPainter( mypPainter );

      // DRAW!
      if ( ! renderLayer( ml, mRenderContext ) )
        emit drawError( ml );

      // composite the cached image into our view and then clean up from caching
      // by reinstating the painter as it was swapped out for caching renders
      delete mRenderContext.painter();
      mRenderContext.setPainter( mypContextPainter );

      // set cache image to the newly rendered image
      mCache->setCacheImage( ml->getLayerID(), cacheImage );
    }

    // draw cached image
    mypContextPainter->drawImage( 0, 0, cacheImage );
  }
  else
  {
    // DRAW!
    if ( ! renderLayer( ml, mRenderContext ) )
      emit drawError( ml );
  }
}


bool QgsMapRenderer::renderLayer( QgsMapLayer* ml, QgsRenderContext& ctx )
{
  //decide if we have to scale the raster
  //this is necessary in case QGraphicsScene is used
  bool scaleRaster = false;
  double rasterScaleFactor = ctx.rasterScaleFactor();
  QgsMapToPixel rasterMapToPixel;
  QgsMapToPixel bk_mapToPixel;

  if ( ml->type() == QgsMapLayer::RasterLayer && fabs( rasterScaleFactor - 1.0 ) > 0.000001 )
  {
    scaleRaster = true;
  }

  if ( scaleRaster )
  {
    bk_mapToPixel = ctx.mapToPixel();
    rasterMapToPixel = ctx.mapToPixel();
    rasterMapToPixel.setMapUnitsPerPixel( ctx.mapToPixel().mapUnitsPerPixel() / rasterScaleFactor );
    rasterMapToPixel.setYMaximum( mSize.height() * rasterScaleFactor );
    ctx.setMapToPixel( rasterMapToPixel );
    ctx.painter()->save();
    ctx.painter()->scale( 1.0 / rasterScaleFactor, 1.0 / rasterScaleFactor );
  }

  // split the rendering into two parts if necessary
  bool split = false;
  QgsRectangle r1, r2;
  if ( ctx.coordinateTransform() )
  {
    r1 = mExtent;
    split = splitLayersExtent( ml, r1, r2 );
    ctx.setExtent( r1 );
  }

  // draw the layer
  if ( !ml->draw( ctx ) )
  {
    return false;
  }

  if ( split )
  {
    ctx.setExtent( r2 );
    if ( !ml->draw( ctx ) )
    {
      return false;
    }
    ctx.setExtent( mExtent ); // return back to the original extent
  }

  if ( scaleRaster )
  {
    ctx.setMapToPixel( bk_mapToPixel );
    ctx.painter()->restore();
  }

  return true;
}


void QgsMapRenderer::renderLabels()
{
  QListIterator<QString> li( mLayerSet );
  QgsCoordinateTransform* ct;
  QgsRectangle r1, r2;

  // render all labels for vector layers in the stack, starting at the base
  li.toBack();
  while ( li.hasPrevious() )
  {
    if ( mRenderContext.renderingStopped() )
    {
      break;
    }

    QString layerId = li.previous();

    // TODO: emit drawingProgress((myRenderCounter++),zOrder.size());
    QgsMapLayer *ml = QgsMapLayerRegistry::instance()->mapLayer( layerId );

    if ( !ml || ( ml->type() != QgsMapLayer::VectorLayer ) )
      continue;

    // only make labels if the layer is visible
    // after scale dep viewing settings are checked
    if ( ml->hasScaleBasedVisibility() && ( ml->minimumScale() > mScale || mScale > ml->maximumScale() ) )
      continue;

    bool split = false;

    if ( hasCrsTransformEnabled() )
    {
      r1 = mExtent;
      split = splitLayersExtent( ml, r1, r2 );
      ct = new QgsCoordinateTransform( ml->srs(), *mDestCRS );
      mRenderContext.setExtent( r1 );
    }
    else
    {
      ct = NULL;
    }

    mRenderContext.setCoordinateTransform( ct );

    ml->drawLabels( mRenderContext );
    if ( split )
    {
      mRenderContext.setExtent( r2 );
      ml->drawLabels( mRenderContext );
    }

  }

}


void QgsMapRenderer::setMapUnits( QGis::UnitType u )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  mScaleCalculator->setMapUnits( u );

  // Since the map units have changed, force a recalculation of the scale.
  updateScale();

  emit mapUnitsChanged();
}

QGis::UnitType QgsMapRenderer::mapUnits() const
{
  return mScaleCalculator->mapUnits();
}

void QgsMapRenderer::onDrawingProgress( int current, int total )
{
  // TODO: emit signal with progress
// QgsDebugMsg(QString("onDrawingProgress: %1 / %2").arg(current).arg(total));
  emit updateMap();
}



void QgsMapRenderer::setProjectionsEnabled( bool enabled )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  if ( mProjectionsEnabled != enabled )
  {
    mProjectionsEnabled = enabled;
    QgsDebugMsg( "Adjusting DistArea projection on/off" );
    mDistArea->setProjectionsEnabled( enabled );
    updateFullExtent();
    emit hasCrsTransformEnabled( enabled );
  }
}

bool QgsMapRenderer::hasCrsTransformEnabled()
{
  return mProjectionsEnabled;
}

void QgsMapRenderer::setDestinationSrs( const QgsCoordinateReferenceSystem& srs )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  QgsDebugMsg( "* Setting destCRS : = " + srs.toProj4() );
  QgsDebugMsg( "* DestCRS.srsid() = " + QString::number( srs.srsid() ) );
  if ( *mDestCRS != srs )
  {
    QgsDebugMsg( "Setting DistArea CRS to " + QString::number( srs.srsid() ) );
    mDistArea->setSourceCrs( srs.srsid() );
    *mDestCRS = srs;
    updateFullExtent();
    emit destinationSrsChanged();
  }
}

const QgsCoordinateReferenceSystem& QgsMapRenderer::destinationSrs()
{
  QgsDebugMsgLevel( "* Returning destCRS", 3 );
  QgsDebugMsgLevel( "* DestCRS.srsid() = " + QString::number( mDestCRS->srsid() ), 3 );
  QgsDebugMsgLevel( "* DestCRS.proj4() = " + mDestCRS->toProj4(), 3 );
  return *mDestCRS;
}


bool QgsMapRenderer::splitLayersExtent( QgsMapLayer* layer, QgsRectangle& extent, QgsRectangle& r2 )
{
  bool split = false;

  if ( hasCrsTransformEnabled() )
  {
    try
    {
      QgsCoordinateTransform tr( layer->srs(), *mDestCRS );

#ifdef QGISDEBUG
      // QgsLogger::debug<QgsRectangle>("Getting extent of canvas in layers CS. Canvas is ", extent, __FILE__, __FUNCTION__, __LINE__);
#endif
      // Split the extent into two if the source CRS is
      // geographic and the extent crosses the split in
      // geographic coordinates (usually +/- 180 degrees,
      // and is assumed to be so here), and draw each
      // extent separately.
      static const double splitCoord = 180.0;

      if ( tr.sourceCrs().geographicFlag() )
      {
        // Note: ll = lower left point
        //   and ur = upper right point
        QgsPoint ll = tr.transform( extent.xMinimum(), extent.yMinimum(),
                                    QgsCoordinateTransform::ReverseTransform );

        QgsPoint ur = tr.transform( extent.xMaximum(), extent.yMaximum(),
                                    QgsCoordinateTransform::ReverseTransform );

        if ( ll.x() > ur.x() )
        {
          extent.set( ll, QgsPoint( splitCoord, ur.y() ) );
          r2.set( QgsPoint( -splitCoord, ll.y() ), ur );
          split = true;
        }
        else // no need to split
        {
          extent = tr.transformBoundingBox( extent, QgsCoordinateTransform::ReverseTransform );
        }
      }
      else // can't cross 180
      {
        extent = tr.transformBoundingBox( extent, QgsCoordinateTransform::ReverseTransform );
      }
    }
    catch ( QgsCsException &cse )
    {
      Q_UNUSED( cse );
      QgsDebugMsg( "Transform error caught" );
      extent = QgsRectangle( -DBL_MAX, -DBL_MAX, DBL_MAX, DBL_MAX );
      r2     = QgsRectangle( -DBL_MAX, -DBL_MAX, DBL_MAX, DBL_MAX );
    }
  }
  return split;
}


QgsRectangle QgsMapRenderer::layerExtentToOutputExtent( QgsMapLayer* theLayer, QgsRectangle extent )
{
  if ( hasCrsTransformEnabled() )
  {
    try
    {
      QgsCoordinateTransform tr( theLayer->srs(), *mDestCRS );
      extent = tr.transformBoundingBox( extent );
    }
    catch ( QgsCsException &cse )
    {
      Q_UNUSED( cse );
      QgsDebugMsg( QString( "Transform error caught: " ).arg( cse.what() ) );
    }
  }
  else
  {
    // leave extent unchanged
  }

  return extent;
}

QgsPoint QgsMapRenderer::layerToMapCoordinates( QgsMapLayer* theLayer, QgsPoint point )
{
  if ( hasCrsTransformEnabled() )
  {
    try
    {
      QgsCoordinateTransform tr( theLayer->srs(), *mDestCRS );
      point = tr.transform( point, QgsCoordinateTransform::ForwardTransform );
    }
    catch ( QgsCsException &cse )
    {
      Q_UNUSED( cse );
      QgsDebugMsg( QString( "Transform error caught: %1" ).arg( cse.what() ) );
    }
  }
  else
  {
    // leave point without transformation
  }
  return point;
}

QgsPoint QgsMapRenderer::mapToLayerCoordinates( QgsMapLayer* theLayer, QgsPoint point )
{
  if ( hasCrsTransformEnabled() )
  {
    try
    {
      QgsCoordinateTransform tr( theLayer->srs(), *mDestCRS );
      point = tr.transform( point, QgsCoordinateTransform::ReverseTransform );
    }
    catch ( QgsCsException &cse )
    {
      QgsDebugMsg( QString( "Transform error caught: %1" ).arg( cse.what() ) );
      throw cse; //let client classes know there was a transformation error
    }
  }
  else
  {
    // leave point without transformation
  }
  return point;
}

QgsRectangle QgsMapRenderer::mapToLayerCoordinates( QgsMapLayer* theLayer, QgsRectangle rect )
{
  if ( hasCrsTransformEnabled() )
  {
    try
    {
      QgsCoordinateTransform tr( theLayer->srs(), *mDestCRS );
      rect = tr.transform( rect, QgsCoordinateTransform::ReverseTransform );
    }
    catch ( QgsCsException &cse )
    {
      QgsDebugMsg( QString( "Transform error caught: %1" ).arg( cse.what() ) );
      throw cse; //let client classes know there was a transformation error
    }
  }
  return rect;
}


void QgsMapRenderer::updateFullExtent()
{
  QgsDebugMsg( "called." );
  QgsMapLayerRegistry* registry = QgsMapLayerRegistry::instance();

  // reset the map canvas extent since the extent may now be smaller
  // We can't use a constructor since QgsRectangle normalizes the rectangle upon construction
  mFullExtent.setMinimal();

  // iterate through the map layers and test each layers extent
  // against the current min and max values
  QStringList::iterator it = mLayerSet.begin();
  while ( it != mLayerSet.end() )
  {
    QgsMapLayer * lyr = registry->mapLayer( *it );
    if ( lyr == NULL )
    {
      QgsDebugMsg( QString( "WARNING: layer '%1' not found in map layer registry!" ).arg( *it ) );
    }
    else
    {
      QgsDebugMsg( "Updating extent using " + lyr->name() );
      QgsDebugMsg( "Input extent: " + lyr->extent().toString() );

      // Layer extents are stored in the coordinate system (CS) of the
      // layer. The extent must be projected to the canvas CS
      QgsRectangle extent = layerExtentToOutputExtent( lyr, lyr->extent() );

      QgsDebugMsg( "Output extent: " + extent.toString() );
      mFullExtent.unionRect( extent );

    }
    it++;
  }

  if ( mFullExtent.width() == 0.0 || mFullExtent.height() == 0.0 )
  {
    // If all of the features are at the one point, buffer the
    // rectangle a bit. If they are all at zero, do something a bit
    // more crude.

    if ( mFullExtent.xMinimum() == 0.0 && mFullExtent.xMaximum() == 0.0 &&
         mFullExtent.yMinimum() == 0.0 && mFullExtent.yMaximum() == 0.0 )
    {
      mFullExtent.set( -1.0, -1.0, 1.0, 1.0 );
    }
    else
    {
      const double padFactor = 1e-8;
      double widthPad = mFullExtent.xMinimum() * padFactor;
      double heightPad = mFullExtent.yMinimum() * padFactor;
      double xmin = mFullExtent.xMinimum() - widthPad;
      double xmax = mFullExtent.xMaximum() + widthPad;
      double ymin = mFullExtent.yMinimum() - heightPad;
      double ymax = mFullExtent.yMaximum() + heightPad;
      mFullExtent.set( xmin, ymin, xmax, ymax );
    }
  }

  QgsDebugMsg( "Full extent: " + mFullExtent.toString() );
}

QgsRectangle QgsMapRenderer::fullExtent()
{
  updateFullExtent();
  return mFullExtent;
}

void QgsMapRenderer::setLayerSet( const QStringList& layers )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  foreach (QString layerId, mLayerSet)
  {
    QgsMapLayer* ml = QgsMapLayerRegistry::instance()->mapLayer(layerId);
    if (ml)
    {
      disconnect(ml, SIGNAL(selectionChanged()), this, SLOT(clearLayerCache()));
      disconnect(ml, SIGNAL(editingStopped()), this, SLOT(clearLayerCache()));
      disconnect(ml, SIGNAL(dataChanged()), this, SLOT(clearLayerCache()));
    }
  }

  mLayerSet = layers;

  foreach (QString layerId, mLayerSet)
  {
    QgsMapLayer* ml = QgsMapLayerRegistry::instance()->mapLayer(layerId);
    if (ml)
    {
      connect(ml, SIGNAL(selectionChanged()), this, SLOT(clearLayerCache()));
      connect(ml, SIGNAL(editingStopped()), this, SLOT(clearLayerCache()));
      connect(ml, SIGNAL(dataChanged()), this, SLOT(clearLayerCache()));
    }
  }

  updateFullExtent();
}

QStringList& QgsMapRenderer::layerSet()
{
  return mLayerSet;
}

void QgsMapRenderer::clearLayerCache()
{
  QgsMapLayer* ml = qobject_cast<QgsMapLayer*>(sender());
  if (mCache)
  {
    if (ml)
    {
      QgsDebugMsg("clearing cache for: "+ml->getLayerID());
      mCache->setCacheImage(ml->getLayerID(), QImage());
    }
    else
    {
      QgsDebugMsg("clearing cache - FOR WHOM ?????\n\n\n\n\n\n");
    }
  }
  else
  {
    QgsDebugMsg("not caching -> not clearing cache");
  }
}

QgsOverlayObjectPositionManager* QgsMapRenderer::overlayManagerFromSettings()
{
  QSettings settings;
  QString overlayAlgorithmQString = settings.value( "qgis/overlayPlacementAlgorithm", "Central point" ).toString();

  QgsOverlayObjectPositionManager* result = 0;

  if ( overlayAlgorithmQString != "Central point" )
  {
    QgsPALObjectPositionManager* palManager = new QgsPALObjectPositionManager();
    if ( overlayAlgorithmQString == "Chain" )
    {
      palManager->setPlacementAlgorithm( "Chain" );
    }
    else if ( overlayAlgorithmQString == "Popmusic tabu chain" )
    {
      palManager->setPlacementAlgorithm( "Popmusic tabu chain" );
    }
    else if ( overlayAlgorithmQString == "Popmusic tabu" )
    {
      palManager->setPlacementAlgorithm( "Popmusic tabu" );
    }
    else if ( overlayAlgorithmQString == "Popmusic chain" )
    {
      palManager->setPlacementAlgorithm( "Popmusic chain" );
    }
    result = palManager;
  }
  else
  {
    result = new QgsCentralPointPositionManager();
  }

  return result;
}

bool QgsMapRenderer::readXML( QDomNode & theNode )
{
  QDomNode myNode = theNode.namedItem( "units" );
  QDomElement element = myNode.toElement();

  // set units
  QGis::UnitType units;
  if ( "meters" == element.text() )
  {
    units = QGis::Meters;
  }
  else if ( "feet" == element.text() )
  {
    units = QGis::Feet;
  }
  else if ( "degrees" == element.text() )
  {
    units = QGis::Degrees;
  }
  else if ( "unknown" == element.text() )
  {
    units = QGis::UnknownUnit;
  }
  else
  {
    QgsDebugMsg( "Unknown map unit type " + element.text() );
    units = QGis::Degrees;
  }
  setMapUnits( units );


  // set extent
  QgsRectangle aoi;
  QDomNode extentNode = theNode.namedItem( "extent" );

  QDomNode xminNode = extentNode.namedItem( "xmin" );
  QDomNode yminNode = extentNode.namedItem( "ymin" );
  QDomNode xmaxNode = extentNode.namedItem( "xmax" );
  QDomNode ymaxNode = extentNode.namedItem( "ymax" );

  QDomElement exElement = xminNode.toElement();
  double xmin = exElement.text().toDouble();
  aoi.setXMinimum( xmin );

  exElement = yminNode.toElement();
  double ymin = exElement.text().toDouble();
  aoi.setYMinimum( ymin );

  exElement = xmaxNode.toElement();
  double xmax = exElement.text().toDouble();
  aoi.setXMaximum( xmax );

  exElement = ymaxNode.toElement();
  double ymax = exElement.text().toDouble();
  aoi.setYMaximum( ymax );

  setExtent( aoi );

  // set projections flag
  QDomNode projNode = theNode.namedItem( "projections" );
  element = projNode.toElement();
  setProjectionsEnabled( element.text().toInt() );

  // set destination CRS
  QgsCoordinateReferenceSystem srs;
  QDomNode srsNode = theNode.namedItem( "destinationsrs" );
  srs.readXML( srsNode );
  setDestinationSrs( srs );

  return true;
}

bool QgsMapRenderer::writeXML( QDomNode & theNode, QDomDocument & theDoc )
{
  // units

  QDomElement unitsNode = theDoc.createElement( "units" );
  theNode.appendChild( unitsNode );

  QString unitsString;

  switch ( mapUnits() )
  {
    case QGis::Meters:
      unitsString = "meters";
      break;
    case QGis::Feet:
      unitsString = "feet";
      break;
    case QGis::Degrees:
      unitsString = "degrees";
      break;
    case QGis::UnknownUnit:
    default:
      unitsString = "unknown";
      break;
  }
  QDomText unitsText = theDoc.createTextNode( unitsString );
  unitsNode.appendChild( unitsText );


  // Write current view extents
  QDomElement extentNode = theDoc.createElement( "extent" );
  theNode.appendChild( extentNode );

  QDomElement xMin = theDoc.createElement( "xmin" );
  QDomElement yMin = theDoc.createElement( "ymin" );
  QDomElement xMax = theDoc.createElement( "xmax" );
  QDomElement yMax = theDoc.createElement( "ymax" );

  QgsRectangle r = extent();
  QDomText xMinText = theDoc.createTextNode( QString::number( r.xMinimum(), 'f' ) );
  QDomText yMinText = theDoc.createTextNode( QString::number( r.yMinimum(), 'f' ) );
  QDomText xMaxText = theDoc.createTextNode( QString::number( r.xMaximum(), 'f' ) );
  QDomText yMaxText = theDoc.createTextNode( QString::number( r.yMaximum(), 'f' ) );

  xMin.appendChild( xMinText );
  yMin.appendChild( yMinText );
  xMax.appendChild( xMaxText );
  yMax.appendChild( yMaxText );

  extentNode.appendChild( xMin );
  extentNode.appendChild( yMin );
  extentNode.appendChild( xMax );
  extentNode.appendChild( yMax );

  // projections enabled
  QDomElement projNode = theDoc.createElement( "projections" );
  theNode.appendChild( projNode );

  QDomText projText = theDoc.createTextNode( QString::number( hasCrsTransformEnabled() ) );
  projNode.appendChild( projText );

  // destination CRS
  QDomElement srsNode = theDoc.createElement( "destinationsrs" );
  theNode.appendChild( srsNode );
  destinationSrs().writeXML( srsNode, theDoc );

  return true;
}

void QgsMapRenderer::setLabelingEngine( QgsLabelingEngineInterface* iface )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  if ( mLabelingEngine )
    delete mLabelingEngine;

  mLabelingEngine = iface;
}


void QgsMapRenderer::setScale( double scale )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  mScale = scale;
}

void QgsMapRenderer::enableOverviewMode( bool isOverview )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  mOverview = isOverview;
}

void QgsMapRenderer::setOutputUnits( OutputUnits u )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  mOutputUnits = u;
}

void QgsMapRenderer::setThreadingEnabled( bool use )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  mThreadingEnabled = use;
}

void QgsMapRenderer::setCachingEnabled( bool enabled )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  if ( mCache && !enabled )
  {
    delete mCache;
    mCache = NULL;
  }
  else if ( !mCache && enabled )
  {
    mCache = new QgsMapRendererCache();
  }

}

void QgsMapRenderer::setAntialiasingEnabled( bool enabled )
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  mAntialiasingEnabled = enabled;
}

void QgsMapRenderer::clearCache()
{
  if ( mDrawing )
  {
    QgsDebugMsg("Ignored --- drawing now!");
    return; // do not allow changes while rendering
  }

  if (mCache)
  {
    mCache->clear();
  }
}

// ------------------------

QgsMapRendererCache::QgsMapRendererCache()
{
  clear();
}

void QgsMapRendererCache::clear()
{
  mExtent.setMinimal();
  mScale = 0;
  mScaleFactor = 0;
  mRasterScaleFactor = 0;
  mCachedImages.clear();
}

bool QgsMapRendererCache::init(QgsRectangle extent, double scale, double scaleFactor, double rasterScaleFactor)
{
  // check whether the params are the same
  if (extent == mExtent &&
      scale == mScale &&
      scaleFactor == mScaleFactor &&
      rasterScaleFactor == mRasterScaleFactor )
    return true;

  // set new params
  mExtent = extent;
  mScale = scale;
  mScaleFactor = scaleFactor;
  mRasterScaleFactor = rasterScaleFactor;

  // invalidate cache
  mCachedImages.clear();

  return false;
}

void QgsMapRendererCache::setCacheImage(QString layerId, const QImage& img)
{
  mCachedImages[layerId] = img;
}

QImage QgsMapRendererCache::cacheImage(QString layerId)
{
  return mCachedImages.value(layerId);
}
